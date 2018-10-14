#include <string.h>
#include <termios.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <getopt.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>

//Custom libraries and header files
#include "SafeSysCalls.h"
#include "Constants.h"
#include "UtilityFunctions.h"

// Global Variables
struct termios defaultMode;
char *readBuffer;
int portNum;
FILE *logFile;
int encryptFlag, portFlag, logFlag;
struct pollfd clientPollArray[2];
int processID;
struct sockaddr_in serverAddress;
struct hostent *server;
int socketfd;
int clientMeta[9];

// Restores to pre-execution environment: frees memory, resets terminal attributes, closes pipes etc.
void restore()
{
    //Free memory
    free(readBuffer);

    //Reset the modes
    tcsetattr(0, TCSANOW, &defaultMode);
}

/* --- Main Function --- */

int main(int argc, char *argv[])
{
    //Options
    int option = 1;

    static struct option shell_options[] = {
        {"port", required_argument, 0, 'p'},
        {"encrypt", no_argument, 0, 'e'},
        {"log", required_argument, 0, 'l'}};

    while ((option = getopt_long(argc, argv, "p:el", shell_options, NULL)) > -1)
    {
        switch (option)
        {
        case 'p':
            portFlag = 1;
            portNum = atoi(optarg);
            break;
        case 'e':
            encryptFlag = 1;
            break;
        case 'l':
            logFlag = 1;

            break;

        default:
            fprintf(stderr, "Usage: %s [--port=portNum] [--encrypt=file.key] [--log]\n", argv[0]);
            exit(1);
        }
    }

    //Check for port flag, else abort with usage message
    if (!portFlag)
    {
        fprintf(stderr, "Usage: %s [--port=portNum] [--encrypt=file.key] [--log]", argv[0]);
        exit(1);
    }

    //Get terminal paramters
    tcgetattr(STDIN_FILENO, &defaultMode);

    //Make copy of current terminal mode
    struct termios projectMode = defaultMode;

    //Replace terminal parameters
    projectMode.c_iflag = ISTRIP;
    projectMode.c_oflag = 0;
    projectMode.c_lflag = 0;

    //Set up new non-canonical input mode with no echo
    //TCSANOW opt does the action immediately
    if (tcsetattr(0, TCSANOW, &projectMode) < 0)
    {
        fprintf(stderr, "Unable to set non-canonical input mode with no echo: %s", strerror(errno));
        exit(1);
    }

    atexit(restore);

    //Allocate memory to readBuffer
    readBuffer = (char *)malloc(sizeof(char) * BUFFERSIZE);

    //Set up connection to socket to communicate with server
    socketfd = socket(AF_INET, SOCK_STREAM, 0); // Create socket
    if (socketfd < 0)
    {
        fprintf(stderr, "Socket creation error: %s\n", strerror(errno));
        exit(1);
    }

    server = gethostbyname("localhost"); // Get host IP address etc.
    if (server == NULL)
    {
        fprintf(stderr, "Error no such host: %s\n", strerror(errno));
        exit(1);
    }

    bzero((char *)&serverAddress, sizeof(serverAddress));                                    //zero out server address
    serverAddress.sin_family = AF_INET;                                                      //
    bcopy((char *)server->h_addr, (char *)&serverAddress.sin_addr.s_addr, server->h_length); //Copies contents of server->h_addr into serverAddress field
    serverAddress.sin_port = htons(portNum);

    if (connect(socketfd, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) //connecting to server
    {
        fprintf(stderr, "Socket connection error: %s\n", strerror(errno));
        exit(1);
    }

    bzero(readBuffer, BUFFERSIZE);

    //Make non-blocking socket for I/O
   // int flags = fcntl (0, F_GETFL);
   // fcntl(socketfd, F_SETFL, flags | O_NONBLOCK);

    //Set up polling
    //For input from keyboard
    clientPollArray[KEYBOARD].fd = STDIN_FILENO;
    clientPollArray[KEYBOARD].events = POLLIN | POLLHUP | POLLERR;;

    //For output from shell (coming through server)
    clientPollArray[SHELL].fd = socketfd;
    clientPollArray[SHELL].events = POLL_IN | POLL_HUP | POLL_ERR;

    //Set up meta
    clientMeta[FORMAT] = DEFAULT;
    clientMeta[PIPE_TO_SHELL_READ] = DEFAULT;
    clientMeta[PIPE_TO_SHELL_WRITE] = DEFAULT;
    clientMeta[PIPE_FROM_SHELL_READ] = DEFAULT;
    clientMeta[PIPE_FROM_SHELL_WRITE] = DEFAULT;
    clientMeta[PROC_ID] = DEFAULT;
    clientMeta[SOCKET] = socketfd;
    clientMeta[SENDER] = CLIENT;
    clientMeta[LOG] = logFlag;

    //Give over handling to Utility Function readOrPoll
    readOrPoll(clientPollArray, readBuffer, clientMeta, logFile);
    exit(0);
}
