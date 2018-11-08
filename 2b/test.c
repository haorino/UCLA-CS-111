// NAME: Rohan Varma
// EMAIL: rvarm1@ucla.edu
// ID: 111111111
//lab2_list.c
#include <stdint.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <getopt.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <signal.h>
#include "SortedList.h"
//pointers to the list and (unitialized) elements of it
SortedListElement_t *elements;
//locking mechanisms
pthread_mutex_t mutex_lock = PTHREAD_MUTEX_INITIALIZER;
int spin_lock = 0;
//arg vars
int num_lists = 1;
int num_threads = 1;
int iters = 1;
int opt_yield = 0;
//an enum for our lock types
typedef enum locks {
  none, spin, mutex
} lock_type;
lock_type lock_in_use = none;
//container for list headers and locks
typedef struct sublist {
  SortedList_t* list; 
  int spinlock;
  pthread_mutex_t mutex_lock;
} sublist;
//multi list with array of sublists
typedef struct multi_list {
  int num_lists;
  sublist* lists; //pointers to the sublists
} multi_list;
multi_list mlist;

//general error handling
void print_ops_and_exit(){
  fprintf(stderr, "Usage: ./lab2_list [--threads=t] [--iterations=i] [--yield={dli}] [--sync={m,s}] \n");
  exit(EXIT_FAILURE);
}
void err_exit(const char *msg, int num) {
  fprintf(stderr, "%s: error = %d, message = %s", msg, num, strerror(num));
  exit(EXIT_FAILURE);
}
//hash func
unsigned long hash(const char * key){
  //Jenkins One At A Time Hash https://en.wikipedia.org/wiki/Jenkins_hash_function
  uint32_t hash, i;
  unsigned int len = strlen(key);
  for(hash = i = 0; i < len; ++i)
    {
      hash += key[i];
      hash += (hash << 10);
      hash ^= (hash >> 6);
    }
  hash += (hash << 3);
  hash ^= (hash >> 11);
  hash += (hash << 15);
  return hash;
}
//spinlock and time
void spin_lock_and_time(int idx, struct timespec * total){
  struct timespec start, end;
  if(clock_gettime(CLOCK_MONOTONIC_RAW, &start) < 0) err_exit("clock gettime errror", errno);
  while (__sync_lock_test_and_set(&mlist.lists[idx].spinlock, 1) == 1) ; //spin
  if(clock_gettime(CLOCK_MONOTONIC_RAW, &end) < 0) err_exit("clock gettime error", errno);
  total->tv_sec += end.tv_sec - start.tv_sec;
  total->tv_nsec += end.tv_nsec - start.tv_nsec;
}

//mutex lock and time
void pthread_mutex_lock_and_time(int idx, struct timespec *total){
  struct timespec start, end; 
  if(clock_gettime(CLOCK_MONOTONIC_RAW, &start) < 0) err_exit("clock gettime errror", errno);
  pthread_mutex_lock(&mlist.lists[idx].mutex_lock);
  if(clock_gettime(CLOCK_MONOTONIC_RAW, &end) < 0) err_exit("clock gettime error", errno);
  total->tv_sec += end.tv_sec - start.tv_sec;
  total->tv_nsec += end.tv_nsec - start.tv_nsec;
}
//generic lock and unlocks
void lock_and_time(int idx, struct timespec * total){
  lock_in_use == spin ? spin_lock_and_time(idx, total) : pthread_mutex_lock_and_time(idx, total);
}
void unlock(int idx){
  lock_in_use == spin ? __sync_lock_release(&mlist.lists[idx].spinlock) : pthread_mutex_unlock(&mlist.lists[idx].mutex_lock);
}

//list lookups, inserts, deletes
void* list_fun(void *arg, struct timespec *total){
  //if(lock_in_use == none) assert(total == NULL);
  //printf("running no locking func \n");
  SortedListElement_t *begin = arg;
  int i;
  for(i = 0; i < iters; i++){
    unsigned long idx = hash((begin + i)->key) % mlist.num_lists;
    if(lock_in_use != none) lock_and_time(idx, total);
    SortedList_insert(mlist.lists[idx].list, begin + i);
    if(lock_in_use != none) unlock(idx);
  }
  int len = 0;
  for(i=0;i<mlist.num_lists;i++){
    if(lock_in_use != none) lock_and_time(i, total);
    int tmp = SortedList_length(mlist.lists[i].list);
    if(tmp == -1){
      fprintf(stderr, "error: SortedList_length indicated that list at index %d is corrupted \n", i);
      exit(2);
    }
    len+=tmp;
    if(lock_in_use != none) unlock(i);
  }
  if(len == -1){
    fprintf(stderr, "error: SortedList_length indicated that list was corrupted\n");
    exit(2);
  }
  for(i = 0; i < iters; i++){
    unsigned long idx = hash((begin + i)->key) % mlist.num_lists;
    if(lock_in_use != none) lock_and_time(idx, total);
    SortedListElement_t* find = SortedList_lookup(mlist.lists[idx].list, begin[i].key);
    if(find == NULL){
      fprintf(stderr, "error: SortedList_lookup failed when it should've succeeded. \n");
      exit(2);
    }
    int x = SortedList_delete(find);
    if(x != 0){
      fprintf(stderr, "error: delete() failed \n");
      exit(2);
    }
    if(lock_in_use != none) unlock(idx);
  }
  //  if(lock_in_use == none) assert(total == NULL);
  return total;
}
//the starter function
void * thread_func(void *arg){
  struct timespec *total = NULL;
  if(lock_in_use != none){
    total = calloc(1, sizeof *total); //TODO
    if(total == NULL) err_exit("calloc", errno);
  }
  return list_fun(arg, total);

}
//seg handler
void handler(int num) {
  if (num == SIGSEGV) {
    fprintf(stderr, "Seg fault caught: error = %d, message =  %s \n", num, strerror(num));
    exit(2);
  }
}

int main(int argc, char ** argv){
  int opt = 0;
  static struct option long_options[] = {
    {"threads", required_argument, 0, 't'},
    {"iterations", required_argument, 0, 'i'},
    {"yield", required_argument, 0, 'y'},
    {"lists", required_argument, 0, 'l'},
    {"sync", required_argument, 0, 's'}
  };
  while((opt = getopt_long(argc, argv, "t:s:", long_options, NULL)) != -1){
    int i;
    switch(opt){
    case 't':
      num_threads = atoi(optarg);
      break;
    case 'i':
      iters = atoi(optarg);
      break;
    case 'l':
      num_lists = atoi(optarg);
      break;
    case 'y':
      for(i=0; optarg[i] != '\0'; i++){
	if(optarg[i] == 'i')
	  opt_yield |= INSERT_YIELD;
	else if(optarg[i] == 'd')
	  opt_yield |= DELETE_YIELD;
	else if(optarg[i] == 'l')
	  opt_yield |= LOOKUP_YIELD;
	else
	  print_ops_and_exit(); //print usage and exit
      }
      break;
    case 's':
      if(strcmp("m", optarg) == 0) lock_in_use = mutex;
      else if(strcmp("s", optarg) == 0) lock_in_use = spin;
      else print_ops_and_exit();
      break;
    default:
      print_ops_and_exit();
    }
  }
  // printf("NUM LISTS: %d \n", num_lists);
  if(signal(SIGSEGV, handler) != NULL) err_exit("signal failed", errno);
  //init empty list
  //TODO
  
  mlist.num_lists = num_lists;
  mlist.lists = calloc(num_lists, sizeof *mlist.lists);
  if(mlist.lists == NULL) err_exit("calloc failed", errno);
  int h;
  for(h=0;h<num_lists;h++){
    sublist * sl = (mlist.lists + h);
    sl->list = malloc(sizeof(SortedList_t));
    sl->list->key = NULL;
    sl->list->next = sl->list->prev = sl->list;
    pthread_mutex_init(&sl->mutex_lock, NULL);
    sl->spinlock = 0;
  }
  long long runs = num_threads * iters;
  //init list elements
  SortedListElement_t *elements = malloc(runs * sizeof(SortedListElement_t));
  //randkeys
  srand(time(NULL));
  int i;
  for(i=0;i<runs;i++){
    int len = rand() % 15 + 1; //keys btwn 1 and 15
    int from_a_offset = rand() % 26;
    char *key = malloc((len+1) * sizeof(char)); //for the null byte
    int j;
    for(j=0;j<len;j++){
      key[j] = 'a' + from_a_offset;
      from_a_offset = rand() % 26;
    }
    key[len] = '\0'; //terminator
    elements[i].key = (const char *) key;
  }
  struct timespec start, end;
  pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
  struct timespec lock_wait;
  memset(&lock_wait, 0, sizeof lock_wait);
  if(clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start) < 0) err_exit("clock_gettime error", errno);
  for(i=0;i<num_threads;i++){
    int x = pthread_create(threads + i, NULL, thread_func, elements + i*iters);
    if(x) err_exit("pthread_create failed", errno);
  }
  for(i=0;i<num_threads;i++){
    void *total_time;
    int x = pthread_join(threads[i], &total_time);
    //if(lock_in_use == none) assert(total_time == NULL);
    if(total_time){
      //      assert(lock_in_use != none);
      lock_wait.tv_sec+=((struct timespec *)total_time)->tv_sec;
      lock_wait.tv_nsec+=((struct timespec *)total_time)->tv_nsec;
    }

    if(x) err_exit("pthread_join failed", errno);
  }
  if(clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end) < 0) err_exit("clock_gettime error", errno);
  long long nanoseconds = (end.tv_sec - start.tv_sec) * 1000000000L + (end.tv_nsec - start.tv_nsec);
  // TODO freeing
  free(threads);
  free(elements);
  long long num_ops = num_threads * iters * 3;
  long long avg = nanoseconds/num_ops;
  long long lock_nsec = lock_wait.tv_nsec + 1000000000L * (long long) lock_wait.tv_sec;
  long long lock_operations = num_threads * (3 * iters + 1); //to account for loc
  long long lock_avg = lock_nsec / lock_operations;
  char *yield_str;
  if(opt_yield == 0) yield_str = "none";
  else if((opt_yield & INSERT_YIELD) && (opt_yield & DELETE_YIELD) && (opt_yield & LOOKUP_YIELD)) yield_str = "idl";
  else if((opt_yield & INSERT_YIELD) && (opt_yield & DELETE_YIELD)) yield_str = "id";
  else if((opt_yield & INSERT_YIELD) && (opt_yield & LOOKUP_YIELD)) yield_str = "il";
  else if(opt_yield & INSERT_YIELD) yield_str = "i";
  else if((opt_yield & DELETE_YIELD) && (opt_yield & LOOKUP_YIELD)) yield_str = "dl";
  else if(opt_yield & DELETE_YIELD) yield_str = "d";
  else yield_str = "l";
  printf("list-%s-%s,%d,%d,%d,%lld,%lld,%lld,%lld\n", yield_str,
	 lock_in_use == none ? "none" : lock_in_use == mutex ? "m" : "s",
	 num_threads, iters, num_lists, num_ops, nanoseconds, avg, lock_avg);
  exit(EXIT_SUCCESS);
}
