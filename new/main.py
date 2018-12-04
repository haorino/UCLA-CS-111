import csv, argparse, sys, os.path

verbose = False
ALLOCATED = 1
UNALLOCATED = 0

class GroupBlockInfo:
    pass

class SuperBlockInfo:
    pass

class Block:
    reserved_blocks = [0, 1, 2]
    def __init__(self, num, parent_inode, offset, free, level_of_indirection = 0, ref_num = None):
        self.__num = num
        self.__parent_inode = parent_inode
        self.__offset = offset
        self.__free = free
        self.__level_of_indirection = level_of_indirection
        if(ref_num != None):
            self.ref_num = ref_num
        assert(self.__num != None)
        assert(self.__level_of_indirection > -1 and self.__level_of_indirection < 4)

    def __str__(self):
        return str("Block #: " + str(self.__num) + " , Parent Inodes: " + str(self.__parent_inode) + " , Logical Offset: " +
              str(self.__offset) + " , Free: " + str(self.__free) + " , level of indirection: " + str(self.__level_of_indirection))

    def is_valid(self):
        if(Block.total_num_blocks is not None):
            return self.__num > 0 and self.__num < Block.total_num_blocks
        console.log("ERR: TOTAL NUM BLOCKS AS REPORTED BY SUPERNODE is NONE")
        sys.exit(1)

    def is_reserved(self):
        if(self.__num in Block.reserved_blocks):
            return True
        else:
            return False

    def is_free(self):
        return self.__free

    def getNum(self):
        return self.__num

    def getOffset(self):
        return self.__offset

    def getIndirectionLevel(self):
        return self.__level_of_indirection

    def getInode(self):
        return self.__parent_inode

    def indirectionToString(self):
        if(self.__level_of_indirection == 0):
            return ''
        elif(self.__level_of_indirection == 1):
            return ' INDIRECT'
        elif(self.__level_of_indirection == 2):
            return ' DOUBLE INDIRECT'
        elif(self.__level_of_indirection == 3):
            return ' TRIPLE INDIRECT'


def insertBlock(block, block_map):
    if(verbose):
        print('Inserting block: ' + str(block))
    if (block.getNum() not in block_map):
        block_map[block.getNum()] = [block]
    else:
        block_map[block.getNum()].append(block)

def getLogicalOffset(numPtrsInBlock, indexInInodeTable):
    if(indexInInodeTable < 12):
        return indexInInodeTable
    logicalOffset = 11
    levelOfIndirection = indexInInodeTable - 11
    while(levelOfIndirection > 0):
        logicalOffset += numPtrsInBlock**(levelOfIndirection - 1)
        levelOfIndirection -= 1
    return logicalOffset

def parseInodeEntry(row, dict):
    firstBlockIndex = 12
    # We do not consider the indirect blocks of the initial inode summaries
    numBlocksInInodeSummary = 15
    inodeNum = row[1]
    type = row[2]
    size = row[10];
    if(type == 's' and size <= 60):
        return
    for i in range(firstBlockIndex, firstBlockIndex+numBlocksInInodeSummary):
        blockNum = int(row[i])
        if(blockNum is not 0):
            lastDirectPointerIndex = (firstBlockIndex + numBlocksInInodeSummary) - 4
            if(i > lastDirectPointerIndex):
                insertBlock(Block(blockNum, inodeNum, getLogicalOffset(256, i - firstBlockIndex), False, i - 23), dict)
            else:
                insertBlock(Block(blockNum, inodeNum, getLogicalOffset(256, i - firstBlockIndex), False), dict)

def parseIndirectEntry(row, dict):
    inodeNum = int(row[1])
    blockNumOfDataBlockReferenced = int(row[5])
    logicalOffset = long(row[3])
    levelOfIndirection = int(row[2])
    block_to_insert = Block(blockNumOfDataBlockReferenced, inodeNum, logicalOffset, False, levelOfIndirection)
    insertBlock(block_to_insert, dict)

def parseRowForBlock(row, dict):
    rowName = row[0]
    if(rowName == 'INODE'):
        parseInodeEntry(row, dict)
    elif(rowName == 'INDIRECT'):
        parseIndirectEntry(row, dict)
    elif(rowName == 'BFREE'):
        blockNum = int(row[1])
        insertBlock(Block(blockNum, None, None, True), dict)
    elif((rowName == 'GROUP') or (rowName == 'IFREE') or (rowName == 'DIRENT') or (rowName == 'SUPERBLOCK')):
        return
    else:
        print("ERROR: INVALID ROW NAME: " + row[0])
        sys.exit(1)

def pull_super_block_data(reader):
    for row in reader:
        if('SUPERBLOCK' in row):
            Block.block_size = int(row[3])
            Block.total_num_blocks = int(row[1])
            super_info = SuperBlockInfo()
            super_info.block_size = int(row[3])
            super_info.total_num_blocks = int(row[1])
            super_info.inode_size = int(row[4])
            super_info.total_num_inodes = int(row[2])
            super_info.first_inode = int(row[7])
            return super_info

def pull_group_block_desc_data(reader):
    for row in reader:
        if('GROUP' in row):
            group_info = GroupBlockInfo()
            group_info.block_bitmap_num = int(row[6])
            group_info.inode_bitmap_num = int(row[7])
            group_info.first_block_of_inodes_num = int(row[8])
            return group_info

def populate_blocks(csvname):
    with open(csvname) as csvfile:
        reader = csv.reader(csvfile, delimiter=',')
        block_map = dict()
        super_info = pull_super_block_data(reader)
        csvfile.seek(0)
        reserved_info = pull_group_block_desc_data(reader)
        # pull in super and group info
        Block.reserved_blocks.append(reserved_info.block_bitmap_num)
        Block.reserved_blocks.append(reserved_info.inode_bitmap_num)
        inode_table_size = (super_info.total_num_inodes * super_info.inode_size)
        last_block_of_inodes_num = reserved_info.first_block_of_inodes_num + (inode_table_size / super_info.block_size) + (0, 1)[inode_table_size % super_info.block_size is not 0]
        reserved_for_inode_table = range(reserved_info.first_block_of_inodes_num, last_block_of_inodes_num)
        Block.reserved_blocks += reserved_for_inode_table
        if(verbose):
            print("RESERVED BLOCKS: " + str(Block.reserved_blocks))
        csvfile.seek(0)
        for row in reader:
            parseRowForBlock(row, block_map)
        return [block_map, super_info.total_num_blocks]

def block_audits(filename):
    [block_map, total_num_blocks] = populate_blocks(filename)
    for block_num, blocks in block_map.items():
        free_block_for_block_num = False
        allocated_block_count = 0
        for block in blocks:
            if(not block.is_free()):
                if(free_block_for_block_num):
                    print("ALLOCATED BLOCK " + str(block.getNum()) + " ON FREELIST")
                    free_block_for_block_num = False
                if(not block.is_valid()):
                    print("INVALID" + block.indirectionToString() + " BLOCK " + str(block.getNum()) + " IN INODE " + str(block.getInode()) + " AT OFFSET " + str(block.getOffset()))
                elif(block.is_reserved()):
                    print("RESERVED" + block.indirectionToString() + " BLOCK " + str(block.getNum()) + " IN INODE " + str(block.getInode()) + " AT OFFSET " + str(block.getOffset()))
                else:
                    # Block is legal, and we should increment a count keeping track of how many blocks of the same block index we have that are legal
                    allocated_block_count += 1
            else:
                if(block.is_reserved()):
                    print("FREE BLOCK " + str(block.getNum()) + " IS RESERVED")
                free_block_for_block_num = True
        if(allocated_block_count > 1):
            # for every legal reference, print a duplicate message
            for block in blocks:
                if(not block.is_free() and not block.is_reserved() and block.is_valid()):
                    print("DUPLICATE" + block.indirectionToString() + " BLOCK " + str(block.getNum()) + " IN INODE " + str(block.getInode()) + " AT OFFSET " + str(block.getOffset()))
    if(verbose):
        print("Checking for unreferenced blocks...")
    # Now, iterate through all possible block numbers.  Since we collect block numbers from the free list, indirect blocks, and inodes
    # any blocks that we haven't already collected have gone unreferenced, excluding the reserved blocks
    for i in range(0, total_num_blocks):
        if i not in Block.reserved_blocks and i not in block_map:
            print("UNREFERENCED BLOCK " + str(i))

def insert_into_inode_map(inode_map, at_inode, value):
    if(at_inode not in inode_map):
        inode_map[at_inode] = [value]
    else:
        inode_map[at_inode].append(value)

inode_links = dict()
inode_map_directory_copy = dict()
freeList = list()
def populate_inodes(csvname):
    global inode_map_directory_copy
    with open(csvname) as csvfile:
        inode_map = dict()
        reader = csv.reader(csvfile, delimiter=',')
        super_block_data = pull_super_block_data(reader)
        csvfile.seek(0)
        for row in reader:
            rowName = row[0]
            inodeNum = int(row[1])
            if(rowName == 'INODE'):
                insert_into_inode_map(inode_map, inodeNum, ALLOCATED)
                inode_links[inodeNum] = int(row[6])
            elif(rowName == 'IFREE'):
                insert_into_inode_map(inode_map, inodeNum, UNALLOCATED)
                freeList.append(inodeNum)
        inode_map_directory_copy = inode_map
        return [inode_map, super_block_data.first_inode, super_block_data.total_num_inodes]

def inode_audits(filename):
    [inodes, first_inode, num_inodes] = populate_inodes(filename)
    # 2 is root dir, and we do want to check for that
    # reserved_inodes = [0,1] + [0, range(3, first_inode)](first_inode > 3)
    global freeList
    for inodeNum, inodeArr in inodes.items():
        freeCount = 0
        allocatedCount = 0
        for inode in inodeArr:
            if(inode == UNALLOCATED):
                freeCount += 1
            elif(inode == ALLOCATED):
                allocatedCount += 1
        if(freeCount > 0 and allocatedCount > 0):
            print("ALLOCATED INODE " + str(inodeNum) + " ON FREELIST")
            freeList.remove(inodeNum)
    for inodeNum in ([2] + range(first_inode, num_inodes + 1)):
        if(inodeNum not in inodes):
            print("UNALLOCATED INODE " + str(inodeNum) + " NOT ON FREELIST")
            freeList.append(inodeNum)

class DirectoryEntry:
	def __init__(self, row):
		self.parent_inode_num = int(row[1])
		self.self_inode_num = int(row[3])
		self.file_name = row[6].rstrip()

def populate_directories(csvname):
    with open(csvname) as csvfile:
        directory_list = list()
        reader = csv.reader(csvfile, delimiter=',')
        super_block_data = pull_super_block_data(reader)
        csvfile.seek(0)
        for row in reader:
            rowName = row[0]
            if(rowName == 'DIRENT'):
                directory_list.append(DirectoryEntry(row))
        return [directory_list, super_block_data.first_inode, super_block_data.total_num_inodes]

def directory_audits(filename):
    links = dict()
    self_to_parent_inode_num = {2: 2}
    [directories, first_inode, num_inodes] = populate_directories(filename)
    for directoryEntry in directories:
        if (directoryEntry.self_inode_num > num_inodes or directoryEntry.self_inode_num < 0):
            print("DIRECTORY INODE " + str(directoryEntry.parent_inode_num) + " NAME " + directoryEntry.file_name + " INVALID INODE " + str(directoryEntry.self_inode_num))
        elif (directoryEntry.self_inode_num in freeList):
            print("DIRECTORY INODE " + str(directoryEntry.parent_inode_num) + " NAME " + directoryEntry.file_name + " UNALLOCATED INODE " + str(directoryEntry.self_inode_num))    
        else:
            links[directoryEntry.self_inode_num] = links.get(directoryEntry.self_inode_num, 0) + 1
            if (directoryEntry.file_name != "'.'" and directoryEntry.file_name != "'..'"):
                self_to_parent_inode_num[directoryEntry.self_inode_num] = directoryEntry.parent_inode_num
    
    for inode in inode_links:
        linkCount = inode_links[inode]
        if inode in links:
            if linkCount != links[inode]:
                print("INODE " + str(inode)+ " HAS " + str(links[inode]) + " LINKS BUT LINKCOUNT IS " + str(linkCount))
        elif linkCount != 0:
            print("INODE " + str(inode)+ " HAS 0 LINKS BUT LINKCOUNT IS " + str(linkCount))

    for directoryEntry in directories:
        if directoryEntry.file_name == "'.'":
            if directoryEntry.parent_inode_num != directoryEntry.self_inode_num:
                print("DIRECTORY INODE " + str(directoryEntry.parent_inode_num) + " NAME '.' LINK TO INODE " + str(directoryEntry.self_inode_num) + " SHOULD BE " + str(directoryEntry.parent_inode_num))
        elif directoryEntry.file_name == "'..'":
            if (directoryEntry.self_inode_num != self_to_parent_inode_num[directoryEntry.parent_inode_num]):
                print("DIRECTORY INODE " + str(directoryEntry.parent_inode_num) + " NAME '..' LINK TO INODE " + str(directoryEntry.self_inode_num) + " SHOULD BE " + str(self_to_parent_inode_num[directoryEntry.parent_inode_num]))

def parseFileArguments():
    argparser = argparse.ArgumentParser(description='Get CSV File Name of file system summary to analyze.')
    argparser.add_argument('filename', nargs=1, help='file name of csv to analyze')
    # REMOVE ON PROGRAM SUBMISSION
    #argparser.add_argument('--verbose', action='store_true', help='whether verbose output should be enabled')
    try:
        return argparser.parse_args()
    except:
        print("Bad arguments detected. Allowable usage: python main.py [filename_to_analyze.csv]")
        sys.exit(1)

def fileNameExists(filename):
    return os.path.exists(filename)

def main():
    args = parseFileArguments()
    filename = args.filename[0]
    if(not fileNameExists(filename)):
        sys.stderr.write("Error : File not found!")
        sys.exit(1)
#    global verbose
#    verbose = args.verbose
#    if(verbose):
#        print("-------Begin Audit-------")
    block_audits(filename)
    inode_audits(filename)
    directory_audits(filename)
#    if(verbose):
#        print("-------End Audit---------")
main()
