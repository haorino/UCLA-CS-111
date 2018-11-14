#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <sys/fcntl.h>
#include <time.h>
#include <poll.h>
#include <mraa.h>

//Global variables
char scale = 'F';
FILE *logFile = NULL;

//Helper functions
void printErrorAndExit(const char *errorMsg, int errorNum)
{
    fprintf(stderr, "Error %s \nError %d: %s \n", errorMsg, errorNum, strerror(errorNum));
    exit(1);
}

void printUsageAndExit(const char *argv0)
{
    fprintf(stderr,
            "Usage: %s [--threads=numOfThreads] [--iterations=numOfIterations] [--yield={dli}] --sync={m,s}]\n", argv0);
    exit(1);
}

void printCurrentTime()
{
    time_t now;
    time(&now);
    struct tm *localTimeNow = localtime(&now);
    if (!localTimeNow)
        printErrorAndExit("converting to local time", errno);
    else
    {
        char printTime[10];
        strftime(printTime, 9, "%H:%M:%S", localTimeNow);
        printf("%s ", printTime);
        if (logFile != NULL)
            fprintf(logFile, "%s ", printTime);
    }
}

void shutDown()
{
    printCurrentTime();
    printf("SHUTDOWN\n");
    if (logFile != NULL)
    {
        fprintf(logFile, "SHUTDOWN\n");
        exit(0);
    }
    exit(0);
}

int main(int argc, char **argv)
{
    // Initializing variables
    int argsLeft;

    long periodInSecs = 1;

    struct pollfd commandsPoll;
    int firstReport = 1;
    mraa_aio_context temperatureSensor;
    mraa_gpio_context button;
    int stopped = 0;
    char buffer[512];

    //Initializing options
    int option_index = 0;
    static struct option long_options[] = {
        {"scale", required_argument, 0, 's'},
        {"period", required_argument, 0, 'p'},
        {"log", required_argument, 0, 'l'}};

    while ((argsLeft = getopt_long(argc, argv, "s:p:l:", long_options, &option_index)) != -1)
    {
        //Check if any valid arguments are left
        if (argsLeft == -1)
            break;

        //Identify which options were specified
        switch (argsLeft)
        {
        case 's':
            if (strlen(optarg) > 1)
                printUsageAndExit(argv[0]);
            else if (optarg[0] == 'C')
                scale = 'C';
            else if (optarg[0] == 'F')
                scale = 'F';
            else
                printUsageAndExit(argv[0]);
            break;

        case 'p':
            //Fix use strtol
            periodInSecs = atoi(optarg);
            break;

        case 'l':
            //Open log file for writing
            logFile = fopen(optarg, "w");
            if (logFile == NULL)
                printErrorAndExit("opening log file", errno);
            break;

        default:
            printUsageAndExit(argv[0]);
        }
    }

    //Initialize hardware
    temperatureSensor = mraa_aio_init(0);
    button = mraa_aio_init(3);
    if (!temperatureSensor)
        printErrorAndExit("initializing temperature sensor", errno);
    if (!button)
        printErrorAndExit("initializing button", errno);

    //Set up polling for commands
    commandsPoll.fd = STDIN_FILENO;
    commandsPoll.events = POLLIN;

    //Initialize start time
    struct timespec startTime, currentTime;
    if (clock_gettime(CLOCK_REALTIME, &startTime) < 0)
        printErrorAndExit("starting timer for period", errno);

    while (1)
    {
        //Read temperature data
        //Get current time
        if (clock_gettime(CLOCK_REALTIME, &currentTime) < 0)
            printErrorAndExit("getting current time", errno);

        //If first report, don't check period or if period hasn't elapsed
        if (!stopped && (firstReport || ((currentTime.tv_sec - startTime.tv_sec) >= periodInSecs)))
        {
            float currentTemperature = getTemperature(mraa_aio_read(temperatureSensor));
            printCurrentTime();
            printf("%.1f \n", currentTemperature);
            if (logFile != NULL)
                fprintf(logFile, "%.1f \n", currentTemperature);

            if (firstReport)
                firstReport = 0;
        }

        //Check for button presses
        int buttonPressed = mraa_gpio_read(button);
        if (buttonPressed)
            shutDown();

        //Check for commands and if new commands have been issued -> bring changes into effect
        int pendingCommands = poll(&commandsPoll, 2, 0);

        if (pendingCommands < 0)
            printErrorAndExit("polling for commands", errno);

        if (pendingCommands == 0)
            continue;

        //Process any input
        if (commandsPoll.revents & POLLIN)
        {
            //Read from STDIN
            int numBytes = read(STDIN_FILENO, buffer, 512);
            if (numBytes < 0)
                printErrorAndExit("reading commands from stdin", errno);
            else if (numBytes == 0)
                continue;

            //Process multiple commands possibly
            int i, startOfCommand;
            for (i = 0; i < numBytes; i++)
            {
                if (buffer[i] == '\n') //Command completed - process it
                {
                    buffer[i] = '\0'; //Setting null character at newline allows to 'end' string there
                    if (strcmp(buffer + startOfCommand, "OFF") == 0)
                        shutDown();
                    else if (strcmp(buffer + startOfCommand, "STOP") == 0)
                        stopped = 1;
                    else if (strcmp(buffer + startOfCommand, "STOP") == 0)
                        stopped = 0;
                    else if (strcmp(buffer + startOfCommand, "SCALE=C") == 0)
                        scale = 'C';
                    else if (strcmp(buffer + startOfCommand, "SCALE=F") == 0)
                        scale = 'F';
                    else if (strcmp(buffer + startOfCommand, "PERIOD=", 7) == 0)
                    {
                        //Check if valid period
                        long newPeriod = strtol(buffer + startOfCommand + 7, NULL, 10);
                        char *endingPtr;
                        if ((errno == ERANGE && (newPeriod == LONG_MAX || newPeriod == LONG_MIN)) || (errno != 0 && newPeriod == 0))
                            printErrorAndExit("invalid number", errno);
                        else if (endingPtr == buffer + startOfCommand + 7)
                            printErrorAndExit("no period specified", errno);
                        else if (newPeriod < 0)
                            printErrorAndExit("period cannot be negative", errno);

                        //Change period
                        periodInSecs = newPeriod;
                        if (logFile != NULL)
                            fprintf(logFile, "PERIOD=%lu\n", newPeriod);
                    }
                    startOfCommand = i + 1;
                }
            }
        }
    }

    //Close log file
    if (logFile != NULL)
        if (fclose(logFile) != 0)
            printErrorAndExit("closing log file", errno);
            
    //Deinitialize Sensors
    mraa_aio_close(temperatureSensor);
    mraa_gpio_close(button);

    return 0;
}