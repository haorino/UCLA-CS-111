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
pthread_mutex_t mutexLockForListOps;
int spinLock = 0;
SortedListElement_t *elementsArray;
SortedList_t *list;
int totalRuns;
int opt_yield;

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
        SortedList_insert(list, elementsArray + i);

    //Check Length
    if (SortedList_length(list) == -1)
        listCorruptedExit("SortedList_length");

    //Lookup each element and delete it
    for (i = *(int *)threadID; i < totalRuns; i += numOfThreads)
    {
        SortedListElement_t *currentElement = SortedList_lookup(list, elementsArray[i].key);
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
        pthread_mutex_lock(&mutexLockForListOps);

        //Insert element
        SortedList_insert(list, elementsArray + i);

        //Release lock
        pthread_mutex_unlock(&mutexLockForListOps);
    }

    //Check Length

    //Obtain lock
    pthread_mutex_lock(&mutexLockForListOps);
    //Get length
    int length = SortedList_length(list);
    //Release Lock
    pthread_mutex_unlock(&mutexLockForListOps);

    if (length == -1)
        listCorruptedExit("SortedList_length");

    //Lookup each element and delete it
    for (i = *(int *)threadID; i < totalRuns; i += numOfThreads)
    {
        //Obtain lock
        pthread_mutex_lock(&mutexLockForListOps);

        //Lookup
        SortedListElement_t *currentElement = SortedList_lookup(list, elementsArray[i].key);
        if (currentElement == NULL)
            listCorruptedExit("SortedList_lookup");

        //Delete
        if (SortedList_delete(currentElement) != 0)
            listCorruptedExit("SortList_delete");

        //Release lock
        pthread_mutex_unlock(&mutexLockForListOps);
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
        while (__sync_lock_test_and_set(&spinLock, 1) == 1)
            ; //Spin

        //Insert element
        SortedList_insert(list, elementsArray + i);

        //Release lock
        __sync_lock_release(&spinLock);
    }

    //Check Length

    //Obtain lock
    while (__sync_lock_test_and_set(&spinLock, 1) == 1)
        ; //Spin
    //Get length
    int length = SortedList_length(list);
    //Release Lock
    __sync_lock_release(&spinLock);

    if (length == -1)
        listCorruptedExit("SortedList_length");

    //Lookup each element and delete it
    for (i = *(int *)threadID; i < totalRuns; i += numOfThreads)
    {
        //Obtain lock
        while (__sync_lock_test_and_set(&spinLock, 1) == 1)
            ; //Spin

        //Lookup
        SortedListElement_t *currentElement = SortedList_lookup(list, elementsArray[i].key);
        if (currentElement == NULL)
            listCorruptedExit("SortedList_lookup");

        //Delete
        if (SortedList_delete(currentElement) != 0)
            listCorruptedExit("SortList_delete");

        //Release lock
        __sync_lock_release(&spinLock);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    // Initializing variables
    int argsLeft, i;
    numOfThreads = 1;
    numOfIterations = 1;
    opt_yield = 0;
    lockType = 'n';

    //Initializing options
    int option_index = 0;
    static struct option long_options[] = {
        {"threads", required_argument, 0, 't'},
        {"iterations", required_argument, 0, 'i'},
        {"yield", required_argument, 0, 'y'},
        {"sync", required_argument, 0, 's'}};

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
            for (i = 0; i < strlen(optarg); i++)
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

        default:
            printUsageAndExit(argv[0]);
        }
    }

    //Calculate total runs
    totalRuns = numOfIterations * numOfThreads;
    //Register signal handler
    if (signal(SIGSEGV, signalHandler) < 0)
        printErrorAndExit("registering signal handler", errno);

    //Initialize and allocate memory for array of pthreads and pthreadIDs
    pthread_t *pthreadsArray;
    pthreadsArray = malloc(numOfThreads * sizeof(pthread_t));
    int *pthreadIDs = malloc(numOfThreads * sizeof(int));
    for (i = 0; i < numOfThreads; i++)
        pthreadIDs[i] = i;

    //Initialize circular doubly linked list
    list = malloc(sizeof(SortedList_t));
    list->next = list->prev = list;

    //Initialize array of elements
    elementsArray = malloc(totalRuns * sizeof(SortedListElement_t));
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

    //Check if length == 0 (expected as all inserted elements have been removed)
    if (SortedList_length(list) < 0)
        printf("%d\n", SortedList_length(list));

    //Calculate time taken for process
    if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &endTime) < 0)
        printErrorAndExit("getting end time", errno);
    long long runTime = (endTime.tv_sec - startTime.tv_sec) * 1000000000L +
                        (endTime.tv_nsec - startTime.tv_nsec);

    //Free memory
    free(pthreadsArray);
    free(elementsArray);
    free(pthreadIDs);
    free(list);

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
           numOfThreads, numOfIterations, 1, numOfOperations, runTime, timePerOperation);

    exit(0);
}
