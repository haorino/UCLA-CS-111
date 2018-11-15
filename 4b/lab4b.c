#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <sys/fcntl.h>
#include <time.h>
#include <poll.h>
#include <mraa.h>
#include <math.h>

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
            "Usage: %s [--period=periodInSecs] [--scale={C,F}] [--log=fileName]\n", argv0);
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

float getTemperature(int rawTemp)
{
    float step1 = 100000.0 * (1023.0 / ((float)rawTemp) - 1.0);
    int step2 = 4275;
    float tempInC = 1.0 / (log(step1 / 100000.0) / step2 + 1 / 298.15) - 273.15;
    if (scale == 'F')
        return (tempInC * 1.8) + 32;
    else
        return tempInC;
}

int main(int argc, char **argv)
{
    // Initializing variables
    int argsLeft;
    long periodInSecs = 1;
    struct pollfd commandsPoll;
    int firstReport = 1;
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
            logFile = fopen(optarg, "a");
            if (logFile == NULL)
                printErrorAndExit("opening log file", errno);
            break;

        default:
            printUsageAndExit(argv[0]);
        }
    }

    //Initialize hardware
    mraa_init();
    mraa_aio_context temperatureSensor;
    temperatureSensor = mraa_aio_init(1);
    if (!temperatureSensor)
        printErrorAndExit("initializing temperature sensor", errno);
    mraa_gpio_context button = mraa_gpio_init(60);
    mraa_gpio_dir(button, MRAA_GPIO_IN);

    if (!button)
        printErrorAndExit("initializing button", errno);

    //Set up polling for commands
    commandsPoll.fd = 0;
    commandsPoll.events = POLLIN;

    //Initialize start time
    //struct timespec startTime, currentTime;
    //if (clock_gettime(CLOCK_REALTIME, &startTime) < 0)
    //  printErrorAndExit("starting timer for period", errno);

    while (1)
    {
        //If not first report, sleep for periodInSecs seconds
        if (!firstReport)
            sleep(periodInSecs);

        if (!stopped)
        {
            float currentTemperature = getTemperature(mraa_aio_read(temperatureSensor));
            printCurrentTime();
            printf("%.1f \n", currentTemperature);
            if (logFile != NULL)
            {
                fprintf(logFile, "%.1f \n", currentTemperature);
                printf(" log successful\n");    
            }
                

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
            int numBytes = read(0, buffer, 512);
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
                    int isPeriod = 0;
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
                    else if (strncmp(buffer + startOfCommand, "PERIOD=", 7) == 0)
                    {
                        //Check if valid period
                        char *endingPtr;
                        long newPeriod = strtol(buffer + startOfCommand + 7, &endingPtr, 10);
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
                        isPeriod = 1;
                    }

                    if (logFile != NULL && !isPeriod)
                        fprintf(logFile, "%s\n", buffer + startOfCommand);
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
