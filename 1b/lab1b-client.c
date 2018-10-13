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

// Global Variables
struct termios defaultMode;
char *readBuffer;
int portNum;
int encryptFlag, portFlag, logFlag;
struct pollfd clientPollArray[2];
int processID;
struct sockaddr_in serverAddress;
struct hostent *server;
int socketfd;

/* --- Utility Functions --- */
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
        {"log", no_argument, 0, 'l'}
        };

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
    socketfd = socket(AF_INET, SOCK_STREAM, 0);     // Create socket
    if (socketfd < 0)
    {
        fprintf(stderr, "Socket creation error: %s\n", strerror(errno));
        exit(1);
    }

    server = gethostbyname("localhost");            // Get host IP address etc.
    if (server == NULL)
    {
        fprintf(stderr, "Error no such host: %s\n", strerror(errno));
        exit(1);
    }

    bzero((char *) &serverAddress, sizeof(serverAddress));  //zero out server address
    serverAddress.sin_family = AF_INET;                     //
    bcopy( (char *) server->h_addr, (char *) &serverAddress.sin_addr.s_addr, server->h_length); //Copies contents of server->h_addr into serverAddress field
    serverAddress.sin_port = htons(portNum);

    if (connect(socketfd, &serverAddress, sizeof(serverAddress)) < 0) //connecting to server
    {
        fprintf(stderr, "Socket connection error: %s\n", strerror(errno));
        exit(1);
    }
    
    bzero(readBuffer, BUFFERSIZE);

    //Set up socket to communicate between the client and the server
    /*
    //Set up polling 
    //For input from keyboard
    clientPollArray[KEYBOARD].fd = STDIN_FILENO;
    clientPollArray[KEYBOARD].events = POLLIN;

    //For output from shell (coming through server)
    // clientPollArray[SHELL].fd = socket
    clientPollArray[SHELL].events = POLL_IN | POLL_HUP | POLL_ERR;

    readOrPoll();
    */
    exit(0);
}
