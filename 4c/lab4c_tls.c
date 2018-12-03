#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <time.h>
#include <pthread.h>
#include <mraa.h>
#include <aio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>
#include <limits.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>

//Global variables
char scale = 'F';
FILE *logFile = NULL;
int socketfd;

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

void shutDown()
{
  char printTime[10];
  time_t now;
  time(&now);
  struct tm *localTimeNow = localtime(&now);
  if (!localTimeNow)
    printErrorAndExit("converting to local time", errno);
  else
    strftime(printTime, 9, "%H:%M:%S", localTimeNow);

  dprintf(socketfd, "%s SHUTDOWN\n", printTime);
    if (logFile != NULL)

      fprintf(logFile, "%s SHUTDOWN\n", printTime);


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
    
    //Connection parameters
    char *id = "105032378";
    char *hostName = "lever.cs.ucla.edu";

    //Checking if sufficient number of arguments
    if (argc < 2)
    {
        fprintf(stderr, "Port number argument is mandatory\n");
        printUsageAndExit(argv[0]);
    }

    int optionIndex = 0;
    static struct option long_options[] = {
        {"id", required_argument, 0, 'i'},
        {"host", required_argument, 0, 'h'},
        {"scale", required_argument, 0, 's'},
        {"period", required_argument, 0, 'p'},
        {"log", required_argument, 0, 'l'},
        {0, 0, 0, 0}};

    while ((argsLeft = getopt_long(argc, argv, "i:h:l:", long_options, &optionIndex)) != -1)
    {
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
            logFile = fopen(optarg, "a+");
            if (logFile == NULL)
                printErrorAndExit("opening log file", errno);
            break;

        case 'i':
            if (strlen(optarg) != 9)
                fprintf(stderr, "Default ID will be used as provided ID is not exactly 9 characters long");
            else
                id = optarg;
            break;

        case 'h':
            hostName = optarg;
            break;

        default:
            printUsageAndExit(argv[0]);
        }
    }

    //Get and check port number argument
    int portNum = atoi(argv[optind]);
    //TODO fix
    if (portNum == 0)
    {
        fprintf(stderr, "Invalid port number\n");
        printUsageAndExit(argv[0]);
    }

    //Initialize hardware
    mraa_init();
    mraa_aio_context temperatureSensor;
    temperatureSensor = mraa_aio_init(1);
    if (!temperatureSensor)
        printErrorAndExit("initializing temperature sensor", errno);

    //Set up connection to server
    //Initialize socket
    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd < 0)
        printErrorAndExit("initializing socket file descriptor", errno);

    //Initialize server config
    struct hostent *server = gethostbyname(hostName);
    if (server == NULL)
        printErrorAndExit("getting host from name provided", errno);

    //Connect to server
    struct sockaddr_in serverAddress;
    serverAddress.sin_port = htons(portNum);
    serverAddress.sin_family = AF_INET;
    memcpy((char *)&serverAddress.sin_addr.s_addr, (char *)server->h_addr, server->h_length);

    if (connect(socketfd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
        printErrorAndExit("connecting socket", errno);

    //Log ID
    dprintf(socketfd, "ID=%s\n", id);
    if (logFile != NULL)
        fprintf(logFile, "ID=%s\n", id);

    //Set up polling for commands
    commandsPoll.fd = socketfd;
    commandsPoll.events = POLLIN;

    //Initialize start time
    time_t previousTime, currentTime;
    previousTime = time(NULL);
    while (1)
    {
        currentTime = time(NULL);
	const long long previousTimeSecs = previousTime;
	const long long currentTimeSecs = currentTime;
        if (!stopped && (firstReport || currentTimeSecs - previousTimeSecs >= periodInSecs))
        {
	    previousTime = time(NULL);
            float currentTemperature = getTemperature(mraa_aio_read(temperatureSensor));

	    //Get local time and convert to string format
	    char printTime[10];
	    time_t now;
	    time(&now);
	    struct tm *localTimeNow = localtime(&now);
	    if (!localTimeNow)
	      printErrorAndExit("converting to local time", errno);
	    else
	      strftime(printTime, 9, "%H:%M:%S", localTimeNow);
	    
            dprintf(socketfd, "%s %.1f\n", printTime, currentTemperature);
            if (logFile != NULL)
            {
	        fprintf(logFile, "%s %.1f\n", printTime, currentTemperature);
                fflush(logFile);
            }

            if (firstReport)
                firstReport = 0;
        }

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
            int numBytes = read(socketfd, buffer, 512);
            if (numBytes < 0)
                printErrorAndExit("reading commands from stdin", errno);
            else if (numBytes == 0)
                continue;

            //Process multiple commands possibly
            int i, startOfCommand;
            startOfCommand = 0;
            for (i = 0; i < numBytes; i++)
            {
                if (buffer[i] == '\n') //Command completed - process it
                {
                    int isPeriod = 0;
                    buffer[i] = '\0'; //Setting null character at newline allows to 'end' string there
                    if (strcmp(buffer + startOfCommand, "OFF") == 0)
                    {
                        if (logFile != NULL)
                        {
                            fprintf(logFile, "OFF\n");
                            fflush(logFile);
                        }
                        shutDown();
                    }
                    else if (strcmp(buffer + startOfCommand, "STOP") == 0)
                        stopped = 1;
                    else if (strcmp(buffer + startOfCommand, "START") == 0)
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
                        {
                            fprintf(logFile, "PERIOD=%lu\n", newPeriod);
                            fflush(logFile);
                        }
                        isPeriod = 1;
                    }

                    if (logFile != NULL && !isPeriod)
                    {
                        fprintf(logFile, "%s\n", buffer + startOfCommand);
                        fflush(logFile);
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

    return 0;
}
