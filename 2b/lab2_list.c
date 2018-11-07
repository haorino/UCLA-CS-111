// Lab 2A: CS 111
// Siddharth Joshi

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

//Global variables
char lockType;
long long numOfIterations;
int numOfThreads;
pthread_mutex_t *mutexLocksForListOps;
int *spinLocks;
SortedListElement_t *elementsArray;
SortedList_t *hashTable;
long* hashOfElement;
int totalRuns;
int opt_yield;
int numOfLists;

//Helper functions
void printErrorAndExit(const char *errorMsg, int errorNum)
{
    fprintf(stderr, "Error %s \nError %d: %s \n", errorMsg, errorNum, strerror(errorNum));
    exit(1);
}

void listCorruptedExit(const char *funcName)
{
    fprintf(stderr, "Error: %s function indicated that the list was corrupted.\n", funcName);
    exit(2);
}

void printUsageAndExit(char *argv0)
{
    fprintf(stderr,
            "Usage: %s [--threads=numOfThreads] [--iterations=numOfIterations] [--yield={dli}] --sync={m,s}]\n", argv0);
    exit(1);
}

//Implementing Robert Jenking's One At A Time Hash Function 
unsigned long  hash(const char*  key)
{
  size_t i = 0;
  unsigned long hash = 0;
  size_t length = strlen(key);
  while (i != length) {
    hash += key[i++];
    hash += hash << 10;
    hash ^= hash >> 6;
  }
  hash += hash << 3;
  hash ^= hash >> 11;
  hash += hash << 15;
  return hash % numOfLists;
}

void generateRandomKeys(SortedListElement_t *elementsArray)
{
    srand(time(NULL) + 123);
    int i, j;
    for (i = 0; i < totalRuns; i++)
    {
        int length = rand() % 10 + 1; // 1 <= length <= 10
        char *newKey = malloc((length + 1) * sizeof(char));
        for (j = 0; j < length; j++)
        {
            int offset = rand() % 26;
            newKey[j] = 'a' + offset;
        }
        newKey[length] = '\0';
        elementsArray[i].key = newKey;
	hashOfElement[i] = hash(newKey);
    }
    return;
}



//Signal handler for segfault
void signalHandler(int sigNum)
{
    if (sigNum == SIGSEGV)
        listCorruptedExit("Segmentation fault during a list operation");
}

//Different sync mechanisms
void *(*listOpsGeneral)(void *threadID); //Function ptr for different sync mechanisms listops

//Regular list operations
void *listOpsRegular(void *threadID)
{
    int i;
    //Insertion
    for (i = *(int *)threadID; i < totalRuns; i += numOfThreads)
        SortedList_insert(hashTable + hashOfElement[i], elementsArray + i);

    //Check Length
  //  if (SortedList_length(hashTable + hashOfElement[i]) == -1)
    //    listCorruptedExit("SortedList_length");

    //Lookup each element and delete it
    for (i = *(int *)threadID; i < totalRuns; i += numOfThreads)
    {
        SortedListElement_t *currentElement = SortedList_lookup(hashTable + hashOfElement[i], elementsArray[i].key);
        if (currentElement == NULL)
            listCorruptedExit("SortedList_lookup");

        if (SortedList_delete(currentElement) != 0)
            listCorruptedExit("SortList_delete");
    }

    return NULL;
}

//Using mutex
void *listOpsMutex(void *threadID)
{
    int i;
    //Insertion
    for (i = *(int *)threadID; i < totalRuns; i += numOfThreads)
    {
        //Obtain lock
        pthread_mutex_lock(&mutexLocksForListOps[hashOfElement[i]]);

        //Insert element
        SortedList_insert(hashTable + hashOfElement[i], elementsArray + i);

        //Release lock
        pthread_mutex_unlock(&mutexLocksForListOps[hashOfElement[i]]);
    }

    //Check Length

    //Obtain lock
    pthread_mutex_lock(&mutexLocksForListOps[hashOfElement[i]]);
    //Get length
   // int length = SortedList_length(hashTable + hashOfElement[i]);
    //Release Lock
    pthread_mutex_unlock(&mutexLocksForListOps[hashOfElement[i]]);

  //  if (length == -1)
       // listCorruptedExit("SortedList_length");

    //Lookup each element and delete it
    for (i = *(int *)threadID; i < totalRuns; i += numOfThreads)
    {
        //Obtain lock
        pthread_mutex_lock(&mutexLocksForListOps[hashOfElement[i]]);

        //Lookup
        SortedListElement_t *currentElement = SortedList_lookup(hashTable + hashOfElement[i], elementsArray[i].key);
        if (currentElement == NULL)
            listCorruptedExit("SortedList_lookup");

        //Delete
        if (SortedList_delete(currentElement) != 0)
            listCorruptedExit("SortList_delete");

        //Release lock
        pthread_mutex_unlock(&mutexLocksForListOps[hashOfElement[i]]);
    }
    return NULL;
}

//Using custom spin lock
void *listOpsSpinLock(void *threadID)
{
    int i;

    //Insertion
    for (i = *(int *)threadID; i < totalRuns; i += numOfThreads)
    {
        //Obtain lock
        while (__sync_lock_test_and_set(&spinLocks[hashOfElement[i]], 1) == 1)
            ; //Spin

        //Insert element
        SortedList_insert(hashTable + hashOfElement[i], elementsArray + i);

        //Release lock
        __sync_lock_release(&spinLocks[hashOfElement[i]]);
    }

    //Check Length

    //Obtain lock
    while (__sync_lock_test_and_set(&spinLocks[hashOfElement[i]], 1) == 1)
        ; //Spin
    //Get length
   // int length = SortedList_length(hashTable + hashOfElement[i]);
    //Release Lock
    __sync_lock_release(&spinLocks[hashOfElement[i]]);

  //  if (length == -1)
    //    listCorruptedExit("SortedList_length");

    //Lookup each element and delete it
    for (i = *(int *)threadID; i < totalRuns; i += numOfThreads)
    {
        //Obtain lock
        while (__sync_lock_test_and_set(&spinLocks[hashOfElement[i]], 1) == 1)
            ; //Spin

        //Lookup
        SortedListElement_t *currentElement = SortedList_lookup(hashTable + hashOfElement[i], elementsArray[i].key);
        if (currentElement == NULL)
            listCorruptedExit("SortedList_lookup");

        //Delete
        if (SortedList_delete(currentElement) != 0)
            listCorruptedExit("SortList_delete");

        //Release lock
        __sync_lock_release(&spinLocks[hashOfElement[i]]);
    }
    return NULL;
}

//Main function
int main(int argc, char *argv[])
{
    // Initializing variables
    int argsLeft, i;
    numOfThreads = 1;
    numOfIterations = 1;
    opt_yield = 0;
    lockType = 'n';
    numOfLists = 1;

    //Initializing options
    int option_index = 0;
    static struct option long_options[] = {
        {"threads", required_argument, 0, 't'},
        {"iterations", required_argument, 0, 'i'},
        {"yield", required_argument, 0, 'y'},
        {"sync", required_argument, 0, 's'},
        {"lists", required_argument, 0, 'l'},
    };

    while ((argsLeft = getopt_long(argc, argv, "t:i:s:", long_options, &option_index)) != -1)
    {
        //Check if any valid arguments are left
        if (argsLeft == -1)
            break;

        //Identify which options were specified
        switch (argsLeft)
        {
        case 't':
            numOfThreads = atoi(optarg);
            break;

        case 'i':
            numOfIterations = strtoll(optarg, NULL, 10);
            break;

        case 'y':
            if (strlen(optarg) > 3)
                printUsageAndExit(argv[0]);
            int i;
            for (i = 0; optarg[i] != '\0'; i++)
            {
                if (optarg[i] == 'i')
                    opt_yield |= INSERT_YIELD;
                else if (optarg[i] == 'd')
                    opt_yield |= DELETE_YIELD;
                else if (optarg[i] == 'l')
                    opt_yield |= LOOKUP_YIELD;
                else
                    printUsageAndExit(argv[0]);
            }
            break;

        case 's':
            if (strlen(optarg) != 1 && (strcmp(optarg, "m") || strcmp(optarg, "s")))
                printUsageAndExit(argv[0]);
            else
                lockType = optarg[0];
            break;

        case 'l':
            numOfLists = atoi(optarg);
            break;

        default:
            printUsageAndExit(argv[0]);
        }
    }

    //Calculate total runs
    totalRuns = numOfIterations * numOfThreads;
    //Register signal handler
    if (signal(SIGSEGV, signalHandler) == SIG_ERR)
        printErrorAndExit("registering signal handler", errno);

    //Initialize and allocate memory for array of pthreads and pthreadIDs
    pthread_t *pthreadsArray;
    pthreadsArray = malloc(numOfThreads * sizeof(pthread_t));
    int *pthreadIDs = malloc(numOfThreads * sizeof(int));
    for (i = 0; i < numOfThreads; i++)
        pthreadIDs[i] = i;

    //Initialize array of elements
    elementsArray = (SortedListElement_t *) malloc(totalRuns * sizeof(SortedListElement_t));
    hashOfElement = (long *) malloc(totalRuns * sizeof(unsigned long));
    generateRandomKeys(elementsArray);

    //Set type of listOps to be used
    switch (lockType)
    {
    case 's':
        listOpsGeneral = &listOpsSpinLock;
        break;
    case 'm':
        listOpsGeneral = &listOpsMutex;
        break;
    default:
        listOpsGeneral = &listOpsRegular;
        break;
    }

    //Initialize Dynamic Array of List Ptrs (i.e. an open Hash Table)
    //Allocate memory
    hashTable = (SortedList_t *) calloc(numOfLists, sizeof(SortedList_t));
    mutexLocksForListOps = (pthread_mutex_t *) calloc(numOfLists, sizeof(pthread_mutex_t));
    spinLocks = (int *) malloc(numOfLists * sizeof(int));
    //Initialize values
    for (i = 0; i < numOfLists; i++)
    {
        //Initialize circular doubly linked list
        hashTable[i].next = hashTable[i].prev = hashTable + i;

        //Initialize mutex
        if (pthread_mutex_init(&mutexLocksForListOps[i], NULL) != 0)
            printErrorAndExit("initializing mutex", errno);

        //Initialize spinLock
        spinLocks[i] = 0;
    }

    //Initialize timer
    struct timespec startTime, endTime;
    if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &startTime) < 0)
        printErrorAndExit("getting start time", errno);

    //Create threads
    for (i = 0; i < numOfThreads; i++)
    {
        if (pthread_create(&pthreadsArray[i], NULL, listOpsGeneral, &pthreadIDs[i]) != 0)
            printErrorAndExit("creating pthread", errno);
    }

    //Join threads
    for (i = 0; i < numOfThreads; i++)
    {
        if (pthread_join(pthreadsArray[i], NULL) != 0)
            printErrorAndExit("joining pthread", errno);
    }

    //Destroy mutexes
    for (i = 0; i < numOfLists; i++)
    {
        if (pthread_mutex_destroy(&mutexLocksForListOps[i]) != 0)
            printErrorAndExit("destroying mutex", errno);
    }

    //Calculate time taken for process
    if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &endTime) < 0)
        printErrorAndExit("getting end time", errno);
    long long runTime = (endTime.tv_sec - startTime.tv_sec) * 1000000000L +
                        (endTime.tv_nsec - startTime.tv_nsec);

    //Free memory
    // free(pthreadsArray);
    // free(elementsArray);
    // free(pthreadIDs);
    // free(hashTable);
    // free(mutexLocksForListOps);
    // free(spinLocks);

    //Print output
    //Determining the yield string
    char *yieldPrint;
    if (opt_yield == 0)
        yieldPrint = "none";
    else if ((opt_yield & INSERT_YIELD) && (opt_yield & DELETE_YIELD) && (opt_yield & LOOKUP_YIELD))
        yieldPrint = "idl";
    else if ((opt_yield & INSERT_YIELD) && (opt_yield & DELETE_YIELD))
        yieldPrint = "id";
    else if ((opt_yield & INSERT_YIELD) && (opt_yield & LOOKUP_YIELD))
        yieldPrint = "il";
    else if ((opt_yield & DELETE_YIELD) && (opt_yield & LOOKUP_YIELD))
        yieldPrint = "dl";
    else if (opt_yield & INSERT_YIELD)
        yieldPrint = "i";
    else if (opt_yield & DELETE_YIELD)
        yieldPrint = "d";
    else
        yieldPrint = "l";

    //Final calculations
    long long numOfOperations = totalRuns * 3;
    long long timePerOperation = runTime / numOfOperations;

    //Actual output
    printf("list-%s-%s,%d,%lld,%d,%lld,%lld,%lld\n", yieldPrint,
           lockType == 'n' ? "none" : lockType == 's' ? "s" : "m",
           numOfThreads, numOfIterations, numOfLists, numOfOperations, runTime, timePerOperation);

    exit(0);
}
