#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <time.h>
#include <math.h>

#include "ext2_fs.h"
typedef struct super_info
{
    unsigned int inodes_count;
    unsigned int blocks_count;
    unsigned int block_size;
    unsigned int inode_size;
    unsigned int blocks_per_group;
    unsigned int inodes_per_group;
    unsigned int first_inode;
    unsigned short errors;
    unsigned int logical_block_offset_doubly_indirect;
    unsigned int logical_block_offset_triply_indirect;
} Info;

typedef struct fs_group
{
    unsigned short group_num;
    unsigned long blocks_count;
    unsigned int inodes_count;
    unsigned int free_inodes_count;
    unsigned int free_blocks_count;
    unsigned int block_bitmap_num;
    unsigned int inode_bitmap_num;
    unsigned int first_inode_block_num;
} Group;

typedef struct inode
{
    unsigned long num;
    char type;
    unsigned int mode;
    unsigned int owner;
    unsigned int group;
    unsigned int link_count;
    char *last_inode_change;
    char *last_modification;
    char *last_access;
    unsigned int file_size;
    unsigned int num_512_byte_blocks;
    unsigned int block_addresses[15];
} Inode;

typedef struct dir_ent
{
    unsigned long parent_inode_num;
    unsigned int logical_byte_offset;
    unsigned int referenced_inode_num;
    unsigned int entry_length;
    unsigned int name_length;
    char *name;
} DirEntry;

const unsigned int ACTION_BOTTOM_LEVEL = 1;
const unsigned int ACTION_MIDDLE_LEVEL = 2;
const unsigned int ACTION_TOP_LEVEL = 4;

typedef struct traversal_info
{
    unsigned long parent_inode_num;
    unsigned int parent_block_num;
    unsigned int block_num;
    unsigned int level_of_indirection;
    unsigned int depth;
    unsigned int file_size;
    // binary field formatted like 0b000
    // 0b(TOPLEVELACTION)(INDIRECTLEVELACTION)(BOTTOMLEVELACTION)
    // 1 means perform action at that level, 0 means don't
    unsigned char mode;
    unsigned int logical_block_offset;
} TraversalInformation;

const unsigned int S_INODE_COUNT_SIZE = 4;      // s_inodes_count
const unsigned int S_BLOCK_COUNT_SIZE = 4;      // s_blocks_count
const unsigned int S_LOG_BLOCK_SIZE_SIZE = 4;   // s_log_block_size
const unsigned int S_INODE_SIZE_SIZE = 2;       // s_inode_size
const unsigned int S_BLOCKS_PER_GROUP_SIZE = 4; // s_blocks_per_group
const unsigned int S_INODES_PER_GROUP_SIZE = 4; // s_inodes_per_group
const unsigned int S_FIRST_INODE_SIZE = 4;      // s_first_ino
const unsigned int S_ERRORS_SIZE = 2;           // s_errors

const unsigned int S_GROUP_NO_SIZE = 2;               // s_block_group_nr
const unsigned int BG_FREE_BLOCKS_COUNT_SIZE = 2;     // bg_free_blocks_count
const unsigned int BG_FREE_INODES_COUNT_SIZE = 2;     // bg_free_inodes_count
const unsigned int BG_BLOCK_BITMAP_PTR_SIZE = 4;      // bg_block_bitmap
const unsigned int BG_INODE_BITMAP_PTR_SIZE = 4;      // bg_inode_bitmap
const unsigned int BG_FIRST_INODE_BLOCK_PTR_SIZE = 4; // bg_inode_table

const unsigned int SUPER_BLOCK_OFFSET = 1024;
const unsigned int GROUP_DESC_BLOCK_OFFSET = 2048;
const unsigned int IMAGE_NAME_MAX_LENGTH = 100;
const unsigned int BUF_SIZE = 64;
const unsigned int GROUP_DESCRIPTOR_TABLE_SIZE = 20;
int fs_fd = -1;
Info super_info; //Super info made global so that block size etc. can be seen by all functions
Group *group_desc_table_arr; // Block Group info array made global so that other important block group global fields can be seen

void open_fs_image(const char *fs_image_name)
{
    fs_fd = open(fs_image_name, O_RDONLY);
    if (fs_fd < 0)
    {
        fprintf(stderr, "Error opening file system image \"%s\"\n", fs_image_name);
        exit(1);
    }
}

void fs_read_offset(void *buf, size_t count, off_t offset)
{
    ssize_t read_in = pread(fs_fd, buf, count, offset);
    if (read_in < 0 || (size_t)read_in < count)
    {
        fprintf(stderr, "Unsuccessful read at offset: %lld. Aborting...\n", (long long) offset);
        exit(2);
    }
}

void buf_read_offset(void *field, void *buf, size_t field_size, off_t offset){
    unsigned int *working_field = field;
    // POTENTIAL ISSUE HERE, since the type of field could be larger than unsigned int, and therefore part of the field could not be cleared
    *working_field = 0;
    size_t i;
    for (i = 0; i < field_size; i++)
    {
        *working_field |= (int) *((unsigned char *)buf + offset + i) << (8 * i);
    }
}

void populate_field_from_buf(void *field, void *buf, size_t field_size, off_t offset)
{
    unsigned int *working_field = field;
    // POTENTIAL ISSUE HERE, since the type of field could be larger than unsigned int, and therefore part of the field could not be cleared
    *working_field = 0;
    fs_read_offset(buf, field_size, offset);
    size_t i;
    for (i = 0; i < field_size; i++)
    {
        *working_field |= *(((unsigned int *)buf) + i) << (8 * i);
    }
    bzero(buf, BUF_SIZE);
}

void *Malloc(size_t size)
{
    void *space = malloc(size);
    if (space == NULL)
    {
        fprintf(stderr, "Memory allocation of %lu bytes for buffer failed! Exiting.\n", size);
        exit(2);
    }
    return space;
}

Info read_super_block()
{
    void *buf = Malloc(BUF_SIZE);
    bzero(buf, BUF_SIZE);
    off_t s_offset = 0;
    populate_field_from_buf(&super_info.inodes_count, buf, S_INODE_COUNT_SIZE, SUPER_BLOCK_OFFSET + s_offset);
    s_offset = S_INODE_COUNT_SIZE;
    populate_field_from_buf(&super_info.blocks_count, buf, S_BLOCK_COUNT_SIZE, SUPER_BLOCK_OFFSET + s_offset);
    unsigned int log_block_size;
    s_offset = 24;
    populate_field_from_buf(&log_block_size, buf, S_LOG_BLOCK_SIZE_SIZE, SUPER_BLOCK_OFFSET + s_offset);
    super_info.block_size = 1024 << log_block_size;
    s_offset = 88;
    populate_field_from_buf(&super_info.inode_size, buf, S_INODE_SIZE_SIZE, SUPER_BLOCK_OFFSET + s_offset);
    s_offset = 32;
    populate_field_from_buf(&super_info.blocks_per_group, buf, S_BLOCKS_PER_GROUP_SIZE, SUPER_BLOCK_OFFSET + s_offset);
    s_offset = 40;
    populate_field_from_buf(&super_info.inodes_per_group, buf, S_INODE_COUNT_SIZE, SUPER_BLOCK_OFFSET + s_offset);
    s_offset = 84;
    populate_field_from_buf(&super_info.first_inode, buf, S_FIRST_INODE_SIZE, SUPER_BLOCK_OFFSET + s_offset);
    printf("SUPERBLOCK,%d,%d,%d,%d,%d,%d,%d\n", super_info.blocks_count, super_info.inodes_count, super_info.block_size, super_info.inode_size, super_info.blocks_per_group, super_info.inodes_per_group, super_info.first_inode);
    super_info.logical_block_offset_doubly_indirect = super_info.block_size / sizeof(u_int32_t);
    super_info.logical_block_offset_triply_indirect = pow(super_info.logical_block_offset_doubly_indirect, 2);
    free(buf);
    return super_info;
}

unsigned long blocks_in_group(unsigned long group_index, unsigned int max_blocks_per_group, unsigned int num_blocks);

Group read_group_block_descriptor(const size_t group_offset, unsigned long group_num)
{
    Group group_block_info;
    void *buf = Malloc(BUF_SIZE);
    bzero(buf, BUF_SIZE);
    group_block_info.group_num = group_num;
    unsigned int total_num_blocks, blocks_per_group;
    off_t s_offset = 32;
    populate_field_from_buf(&blocks_per_group, buf, S_BLOCKS_PER_GROUP_SIZE, SUPER_BLOCK_OFFSET + s_offset);
    populate_field_from_buf(&total_num_blocks, buf, S_BLOCK_COUNT_SIZE, SUPER_BLOCK_OFFSET + 4);
    group_block_info.blocks_count = blocks_in_group(group_num, blocks_per_group, total_num_blocks);
    s_offset = 0;
    populate_field_from_buf(&group_block_info.inodes_count, buf, S_INODE_COUNT_SIZE, SUPER_BLOCK_OFFSET + s_offset);
    off_t bg_offset = 0;
    populate_field_from_buf(&group_block_info.block_bitmap_num, buf, BG_BLOCK_BITMAP_PTR_SIZE, group_offset + bg_offset);
    bg_offset = BG_BLOCK_BITMAP_PTR_SIZE;
    populate_field_from_buf(&group_block_info.inode_bitmap_num, buf, BG_INODE_BITMAP_PTR_SIZE, group_offset + bg_offset);
    bg_offset += BG_INODE_BITMAP_PTR_SIZE;
    populate_field_from_buf(&group_block_info.first_inode_block_num, buf, BG_FIRST_INODE_BLOCK_PTR_SIZE, group_offset + bg_offset);
    bg_offset += BG_FIRST_INODE_BLOCK_PTR_SIZE;
    populate_field_from_buf(&group_block_info.free_blocks_count, buf, BG_FREE_BLOCKS_COUNT_SIZE, group_offset + bg_offset);
    bg_offset += BG_FREE_BLOCKS_COUNT_SIZE;
    populate_field_from_buf(&group_block_info.free_inodes_count, buf, BG_FREE_INODES_COUNT_SIZE, group_offset + bg_offset);
    printf("GROUP,%d,%lu,%d,%d,%d,%d,%d,%d\n", group_block_info.group_num, group_block_info.blocks_count, group_block_info.inodes_count, group_block_info.free_blocks_count, group_block_info.free_inodes_count, group_block_info.block_bitmap_num, group_block_info.inode_bitmap_num, group_block_info.first_inode_block_num);
    free(buf);
    return group_block_info;
}

Group *read_group_block_descriptors(size_t num_descriptors)
{
    Group *bg_desc_structs = Malloc(sizeof(Group) * num_descriptors);
    size_t i;
    for (i = 0; i < num_descriptors; i++)
    {
        bg_desc_structs[i] = read_group_block_descriptor(GROUP_DESC_BLOCK_OFFSET + (i * GROUP_DESCRIPTOR_TABLE_SIZE), i);
    }
    return bg_desc_structs;
}

unsigned long blocks_in_group(unsigned long group_index, unsigned int max_blocks_per_group, unsigned int num_blocks)
{
    if (num_blocks > max_blocks_per_group)
    {
        unsigned int num_groups = (num_blocks / max_blocks_per_group) + 1;
        if (group_index == num_groups - 1)
        {
            return num_blocks % max_blocks_per_group;
        }
        else
        {
            return max_blocks_per_group;
        }
    }
    else
    {
        return num_blocks;
    }
}

short *scan_bitmap(off_t offset, unsigned long bits_long)
{
    short *bitmap = Malloc(sizeof(short) * bits_long);
    signed char *buf = Malloc(BUF_SIZE);
    fs_read_offset(buf, (bits_long / 8) + ((bits_long % 8) == 0 ? 0 : 1), offset);
    size_t bit_index;
    for (bit_index = 0; bit_index < bits_long; bit_index++)
    {
        bitmap[bit_index] = (buf[bit_index / 8] >> (bit_index % 8)) & 1;
    }
    free(buf);
    return bitmap;
}

void free_block_summary(unsigned long block_bitmap_offset, unsigned long num_blocks)
{
    // a.) 1 means used, and 0 free/available
    // b.) The first block of this block group is represented by bit 0 of byte 0, the second by bit 1 of byte 0. The 8th
    //     block is represented by bit 7 (most significant bit) of byte 0 while the 9th block is represented by bit 0
    //     (least significant bit) of byte 1.

    short *block_bitmap = scan_bitmap(block_bitmap_offset, num_blocks);
    size_t i;
    for (i = 0; i < num_blocks; i++)
    {
        if (block_bitmap[i] == 0)
        {
            printf("BFREE,%d\n", (unsigned int)i + 1);
        }
    }
    free(block_bitmap);
}

void free_inode_summary(unsigned long inode_bitmap_offset, unsigned long num_inodes)
{
    // a.) 1 means used, and 0 free/available
    // b.) The first inode of this inode group is represented by bit 0 of byte 0, the second by bit 1 of byte 0. The 8th
    //     inode is represented by bit 7 (most significant bit) of byte 0 while the 9th inode is represented by bit 0
    //     (least significant bit) of byte 1.
    short *inode_bitmap = scan_bitmap(inode_bitmap_offset, num_inodes);
    size_t i;
    for (i = 0; i < num_inodes; i++)
    {
        if (inode_bitmap[i] == 0)
        {
            printf("IFREE,%d\n", (unsigned int)i + 1);
        }
    }
    free(inode_bitmap);
}

off_t offset_from_block(unsigned long block_num, unsigned long block_size)
{
    return block_num * block_size;
}

void inode_summary(const unsigned long offset, const unsigned long group_num, const unsigned long inodes_per_group, unsigned long inode_index);

void used_inodes_summary(off_t inode_table_offset, unsigned long group_num, unsigned long inodes_per_group, unsigned long inode_size){
    size_t i;
    for(i = 0; i < inodes_per_group; i++){
        inode_summary(inode_table_offset + (i*inode_size), group_num, inodes_per_group, i);
    }
}

void get_gmt_time(char* time_str, const time_t* t){
    size_t size_of_format = strlen("mm/dd/yy hh:mm:ss");
    bzero(time_str, size_of_format);
    struct tm *time = Malloc(sizeof(struct tm));
    gmtime_r(t, time);
    if(time == NULL){
        fprintf(stderr, "Supplied time_t year does not fit into integer.\n");
        free(time);
        exit(2);
    }
    size_t num_bytes = strftime(time_str, size_of_format + 1, "%m/%d/%y %H:%M:%S", time);
    if(num_bytes != size_of_format){
        fprintf(stderr, "Incorrect number of bytes placed into time string.\n");
        free(time);
        exit(2);
    }
    free(time);
    time = NULL;
}

void scan_indirect_block_references(TraversalInformation info);
void print_indirect_info(TraversalInformation* t);
void scan_directory(Inode* inode);

void inode_summary(const unsigned long offset, const unsigned long group_num, const unsigned long inodes_per_group, const unsigned long inode_index){
    Inode i;
    void *buf = Malloc(BUF_SIZE);
    bzero(buf, BUF_SIZE);
    populate_field_from_buf(&i.mode, buf, 2, offset);
    populate_field_from_buf(&i.link_count, buf, 2, offset + 26);
    if(i.mode != 0 && i.link_count != 0){
        size_t sizeof_date_str = strlen("mm/dd/yy hh:mm:ss");
        // get inode number
        i.num = (inodes_per_group * group_num) + (inode_index + 1);
        // get file type
        if((i.mode & 0x8000) == 0x8000){
            i.type = 'f';
        }
        else if((i.mode & 0xA000) == 0xA000){
            i.type = 's';
        }
        else if((i.mode & 0x4000) == 0x4000){
            i.type = 'd';
        }
        else{
            i.type = '?';
        }
        // get low order 12 bits of mode
        i.mode = i.mode & 0xFFF;
        // get owner
        populate_field_from_buf(&i.owner, buf, 2, offset + 2);
        // get group
        populate_field_from_buf(&i.group, buf, 2, offset + 24);
        // get time of last inode change mm/dd/yy hh:mm:ss
        time_t last_inode_change = 0;
        i.last_inode_change = Malloc(sizeof_date_str + 1);
        populate_field_from_buf(&last_inode_change, buf, 4, offset + 12);
        get_gmt_time(i.last_inode_change, &last_inode_change);
        // get modification time mm/dd/yy hh:mm:ss
        time_t last_modification = 0;
        i.last_modification = Malloc(sizeof_date_str + 1);
        populate_field_from_buf(&last_modification, buf, 4, offset + 16);
        get_gmt_time(i.last_modification, &last_modification);
        // get time of last access mm/dd/yy hh:mm:ss
        time_t last_access = 0;
        i.last_access = Malloc(sizeof_date_str + 1);
        populate_field_from_buf(&last_access, buf, 4, offset + 8);
        get_gmt_time(i.last_access, &last_access);
        // get file size
        populate_field_from_buf(&i.file_size, buf, 4, offset + 4);
        // get number of (512 byte) blocks of disk space (decimal) taken up by this file
        populate_field_from_buf(&i.num_512_byte_blocks, buf, 4, offset + 28);
        printf("INODE,%lu,%c,%o,%d,%d,%d,%s,%s,%s,%d,%d,", i.num, i.type, i.mode, i.owner, i.group, i.link_count, i.last_inode_change, i.last_modification, i.last_access, i.file_size, i.num_512_byte_blocks);
        // get address information
        size_t j;
        for(j = 0; j < 15; j++){
            populate_field_from_buf(&i.block_addresses[j], buf, 4, offset + 40 + (j * 4));
        }
        if (i.type != 's' || (i.type == 's' && i.file_size > 60)){
            size_t j;
            for(j = 0; j < 14; j++){
                printf("%u,", i.block_addresses[j]);
            }
            printf("%u\n", i.block_addresses[14]);
        }
        if (i.type == 'd'){
            scan_directory(&i); //Call traverse addresses with action as scan_directory
        }
        
        TraversalInformation info;
        info.parent_inode_num = i.num;
        info.level_of_indirection = 1;
        info.parent_block_num = i.block_addresses[12];
        info.logical_block_offset = 12;
        if (i.block_addresses[12] != 0)
            scan_indirect_block_references(info);
        if (i.block_addresses[13] != 0)
        {
            info.level_of_indirection++;
            info.parent_block_num = i.block_addresses[13];
            info.logical_block_offset += super_info.block_size/sizeof(u_int32_t);
            scan_indirect_block_references(info);
        }
        if (i.block_addresses[14] != 0)
        {
            info.level_of_indirection++;
            info.parent_block_num = i.block_addresses[14];
            info.logical_block_offset += pow(super_info.block_size/sizeof(u_int32_t), 2);
            scan_indirect_block_references(info);
        }
    }
    free(buf);
    buf = NULL;
}

//Recursive function to deal with indirect directory entries
void scan_indirect_directory(TraversalInformation info) {
	u_int32_t num_of_entries = super_info.block_size/sizeof(u_int32_t);
	u_int32_t block_data[num_of_entries];
	memset(block_data, 0, sizeof(block_data));
    fs_read_offset(block_data, super_info.block_size, offset_from_block(info.block_num, super_info.block_size));

	unsigned char entries_or_blocks[super_info.block_size];
	struct ext2_dir_entry *current_entry;
    int i;
	for (i = 0; (u_int32_t) i < num_of_entries; i++) {
		if (block_data[i] == 0) 
            return;

		if (info.level_of_indirection > 1) 
        {
            info.block_num = block_data[i];
            info.level_of_indirection--;
			scan_indirect_directory(info);
		}

		fs_read_offset(entries_or_blocks, super_info.block_size, offset_from_block(info.block_num, super_info.block_size));
		current_entry = (struct ext2_dir_entry *) entries_or_blocks;

		while((info.logical_block_offset < info.file_size) && current_entry->file_type) 
        {
			char file_name[EXT2_NAME_LEN+1];
			memcpy(file_name, current_entry->name, current_entry->name_len);
			file_name[current_entry->name_len] = 0;
			if (current_entry->inode != 0) 
            {
				printf("DIRENT,%d,%d,%d,%d,%d,'%s'\n",
					(int) info.parent_inode_num,
					info.logical_block_offset,
					current_entry->inode,
					current_entry->rec_len,
					current_entry->name_len,
					file_name);
			}
			info.logical_block_offset += current_entry->rec_len;
			current_entry = (void*) current_entry + current_entry->rec_len;
		}
		
	}
}

void scan_directory(Inode* inode) {
	unsigned char entries[super_info.block_size];
	struct ext2_dir_entry *current_entry;
	unsigned int logical_byte_offset = 0;
    TraversalInformation info;
    info.file_size = inode->file_size;
    info.parent_inode_num = inode->num;
    int i;
	for (i = 0; i < 15; i++) {
        if (inode->block_addresses[i] == 0)
            return;

        if (i >= 12)
        {
            info.level_of_indirection = i - 11;
            info.logical_block_offset = logical_byte_offset;
            scan_indirect_directory(info);
        }

		fs_read_offset(entries, super_info.block_size, offset_from_block(inode->block_addresses[i], super_info.block_size));
		current_entry = (struct ext2_dir_entry *) entries;

		while((logical_byte_offset < inode->file_size) && current_entry->file_type) {
			char file_name[EXT2_NAME_LEN + 1];
			memcpy(file_name, current_entry->name, current_entry->name_len);
			file_name[current_entry->name_len] = '\0';
			if (current_entry->inode != 0) {
				printf("DIRENT,%d,%d,%d,%d,%d,'%s'\n",
						(int) inode->num,
						logical_byte_offset,
						current_entry->inode,
						current_entry->rec_len,
						current_entry->name_len,
						file_name);
			}
			logical_byte_offset += current_entry->rec_len;
			current_entry = (void*) current_entry + current_entry->rec_len;
		}
        
    }
}

void scan_indirect_block_references(TraversalInformation info) {
	u_int32_t num_of_references = super_info.block_size/sizeof(u_int32_t);
	u_int32_t block_data[num_of_references];
	memset(block_data, 0, super_info.block_size);

    fs_read_offset(block_data, super_info.block_size, offset_from_block(info.parent_block_num, super_info.block_size));
	int i;
    for (i = 0; (u_int32_t) i < num_of_references; i++)
    {
		if (block_data[i] == 0) 
        {            
            switch(info.level_of_indirection)
            {
                case 1:
                    info.logical_block_offset++;
                    break;
                case 2:
                    info.logical_block_offset += super_info.logical_block_offset_doubly_indirect;
                    break;
                case 3:
                    info.logical_block_offset += super_info.logical_block_offset_triply_indirect;
                    break;
                default:
                    fprintf(stderr, "Error: invalid level of indirection detected\n");
                    exit(1);
            }    
            continue;
        }

        info.block_num = block_data[i];
		print_indirect_info(&info);

		if (info.level_of_indirection == 1)
			info.logical_block_offset++;
		else if (info.level_of_indirection > 1) 
        {
            info.level_of_indirection--;
            info.parent_block_num = info.block_num;
			scan_indirect_block_references(info);
		}
	}
}

void print_indirect_info(TraversalInformation* t){
    printf("INDIRECT,%lu,%d,%d,%d,%d\n",t->parent_inode_num,t->level_of_indirection, t->logical_block_offset, t->parent_block_num, t->block_num);
}

int main(int argc, const char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "You may only pass one argument, which is the name of the filesystem image to analyze.\n");
        exit(1);
    }
    //TODO: Check if image is empty
    open_fs_image(argv[1]);
    super_info = read_super_block();
    unsigned int num_groups = (super_info.blocks_count / super_info.blocks_per_group) + 1;
    group_desc_table_arr = read_group_block_descriptors(num_groups);
    
    unsigned int i;
    for (i = 0; i < num_groups; i++)
    {
        unsigned long num_blocks_in_group = group_desc_table_arr[i].blocks_count;
        unsigned long block_bitmap_offset = offset_from_block(group_desc_table_arr[i].block_bitmap_num, super_info.block_size);
        free_block_summary(block_bitmap_offset, num_blocks_in_group);
        unsigned long num_inodes_in_group = group_desc_table_arr[i].inodes_count;
        unsigned long inode_bitmap_offset = offset_from_block(group_desc_table_arr[i].inode_bitmap_num, super_info.block_size);
        free_inode_summary(inode_bitmap_offset, num_inodes_in_group);
        used_inodes_summary(group_desc_table_arr[i].first_inode_block_num * super_info.block_size, i, num_inodes_in_group, super_info.inode_size);
    }
    return 0;
}
