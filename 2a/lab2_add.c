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

//Global vairables
int yieldFlag;
char lockType;
long long counter;
long long numOfIterations;
pthread_mutex_t sumMutex = PTHREAD_MUTEX_INITIALIZER;
int spinLock;

//Helper functions
void printErrorAndExit(const char *errorMsg, int errorNum)
{
    fprintf(stderr, "Error %s \nError %d: %s \n", errorMsg, errorNum, strerror(errorNum));
    exit(1);
}

void printUsageAndExit(char *argv0)
{
    fprintf(stderr,
            "Usage: %s [--threads=numOfThreads] [--iterations=numOfIterations] [--yield] --sync={m,s,c}]\n", argv0);
    exit(1);
}

//Given add function (yields after summing if yield option specified)
void add(long long *pointer, long long value)
{
    long long sum = *pointer + value;
    if (yieldFlag)
        sched_yield();
    *pointer = sum;
}

//Different sync mechanisms
void (*lockedAdd)(long long *pointer, long long value); //Function ptr different sync mechanisms adds

void mutexAdd(long long *pointer, long long value) //Using mutex
{
    //Try to obtain the lock
    int status;
    if ((status = pthread_mutex_lock(&sumMutex)) != 0)
      printErrorAndExit("locking mutex on sum", status);

    add(pointer, value);

    if ((status = pthread_mutex_unlock(&sumMutex)) != 0)
        fprintf(stderr, "%d\n", status); //printErrorAndExit("unlocking mutex on sum", errno);
}

void spinAdd(long long *pointer, long long value) //Using custom spin lock
{
    while (__sync_lock_test_and_set(&spinLock, 1) == 1)
        ; //spin i.e. keep trying to obtain lock

    add(pointer, value);

    __sync_lock_release(&spinLock);
}

void atomicSyncAdd(long long *pointer, long long value) //Using compare & swap to ensure oldSum is added to
{
    long long oldSum, newSum;
    do
    {
        oldSum = *pointer;
        newSum = oldSum + value;

        if (yieldFlag)
            sched_yield();
    } while (oldSum != __sync_val_compare_and_swap(pointer, oldSum, newSum));
}

//Utility functions
void *iterateOverAdd()
{
    long long i;
    for (i = 0; i < numOfIterations; i++)
        lockedAdd(&counter, 1);
    for (i = 0; i < numOfIterations; i++)
        lockedAdd(&counter, -1);
    return NULL;
}

int main(int argc, char *argv[])
{
    // Initializing variables
    int numOfThreads, argsLeft, i;
    numOfThreads = 1;
    numOfIterations = 1;
    yieldFlag = 0;
    lockType = 'n';
    counter = 0;

    //Initializing options
    int option_index = 0;
    static struct option long_options[] = {
        {"threads", required_argument, 0, 't'},
        {"iterations", required_argument, 0, 'i'},
        {"yield", no_argument, 0, 'y'},
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
            yieldFlag = 1;
            break;

        case 's':
            if (strlen(optarg) != 1 && (strcmp(optarg, "m") || strcmp(optarg, "s") || strcmp(optarg, "c")))
                printUsageAndExit(argv[0]);
            else
                lockType = optarg[0];
            break;

        default:
            printUsageAndExit(argv[0]);
        }
    }

    //Initialize and allocate memory for array of pthreads
    pthread_t *pthreadsArray;
    pthreadsArray = malloc(numOfThreads * sizeof(pthread_t));

    //Set type of add to be used
    switch (lockType)
    {
    case 's':
        lockedAdd = &spinAdd;
        break;
    case 'm':
        lockedAdd = &mutexAdd;
        break;
    case 'c':
        lockedAdd = &atomicSyncAdd;
        break;
    default:
        lockedAdd = &add;
    }

    //Initialize timer
    struct timespec startTime, endTime;
    if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &startTime) < 0)
        printErrorAndExit("getting start time", errno);

    //Create threads
    for (i = 0; i < numOfThreads; i++)
    {
        if (pthread_create(&pthreadsArray[i], NULL, iterateOverAdd, NULL) != 0)
            printErrorAndExit("creating pthread", errno);
    }

    //Join threads
    for (i = 0; i < numOfThreads; i++)
    {
        if (pthread_join(pthreadsArray[i], NULL) != 0)
            printErrorAndExit("joining pthread", errno);
    }

    //Calculate time taken for process
    if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &endTime) < 0)
        printErrorAndExit("getting end time", errno);
    long long runTime = (endTime.tv_sec - startTime.tv_sec) * 1000000000L +
                        (endTime.tv_nsec - startTime.tv_nsec);

    //Free memory
    free(pthreadsArray);

    //Print output
    long long numOfOperations = numOfIterations * numOfThreads * 2;
    long long timePerOperation = runTime / numOfOperations;
    printf("add%s-%s,%d,%lld,%lld,%lld,%lld,%lld\n", yieldFlag ? "-yield" : "",
           lockType == 'n' ? "none" : lockType == 's' ? "s" : lockType == 'm' ? "m" : "c",
           numOfThreads, numOfIterations, numOfOperations, runTime, timePerOperation, counter);

    exit(0);
}
