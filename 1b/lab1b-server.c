#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <getopt.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <poll.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

//Custom libraries and header files
#include "SafeSysCalls.h"
#include "Constants.h"
#include "UtilityFunctions.h"

// Global Variables
char *readBuffer;
int portFlag, encryptFlag, debugFlag;
int portNum;
int pipeToShell[2], pipeFromShell[2];
int initSocketfd, connectedSocketfd;
unsigned int clientAddressLength;
int processID;
int serverMeta[9];
//struct termios *defaultMode;
struct pollfd serverPollArray[2];
struct sockaddr_in serverAddress, cllientAddress;
//object containing internet address

/* --- Utility Functions --- */
// Restores to pre-execution environment: frees memory, closes pipes etc.
void restore()
{
    //Close Pipes

    //Free Memory
}

/* --- Signal Handler --- */
void signal_handler(int sig)
{
    if (sig == SIGPIPE)
    {
        exit(0);
    }
}

/* --- Main Function --- */

int main(int argc, char *argv[])
{
    //Options

    int option = 1;

    static struct option shell_options[] = {
        {"port", required_argument, 0, 'p'},
        {"encrypt", required_argument, 0, 'e'},
        {"debug", no_argument, 0, 'd'}};

    while ((option = getopt_long(argc, argv, "p:e:d", shell_options, NULL)) > -1)
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
        case 'd':
            debugFlag = 1;
            break;
        default:
            fprintf(stderr, "Usage: %s [--port=portNum] [--encrypt=file.key] \n", argv[0]);
            exit(1);
        }
    }

    //Check for port flag, else abort with usage message
    if (!portFlag)
    {
        fprintf(stderr, "Usage: %s [--port=portNum] [--encrypt=file.key] ", argv[0]);
        exit(1);
    }

    atexit(restore);

    //Allocate memory to readBuffer
    readBuffer = (char *)malloc(sizeof(char) * BUFFERSIZE);

    //Set Up Socket to communicate with client process
    initSocketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (initSocketfd < 0)
    {
        fprintf(stderr, "Socket creation failed: %s", strerror(errno));
        exit(1);
    }

    bzero((char *)&serverAddress, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(portNum);

    if (bind(initSocketfd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        fprintf(stderr, "Socket binding error: %s", strerror(errno));
        exit(1);
    }

    listen(initSocketfd, 5);
    clientAddressLength = sizeof(cllientAddress);
    connectedSocketfd = accept(initSocketfd, (struct sockaddr *)&cllientAddress, &clientAddressLength);

    if (connectedSocketfd < 0)
    {
        fprintf(stderr, "Error while accepting connection to socket: %s", strerror(errno));
        exit(1);
    }

    //Zero out buffer
    bzero(readBuffer, BUFFERSIZE);

    //Make non-blocking socket for I/O
    //int flags = fcntl (0, F_GETFL);
   // fcntl(connectedSocketfd, F_SETFL, flags | O_NONBLOCK);

    //Set up pipes to communicate between the shell and the new terminal
    if (pipe(pipeFromShell) < 0 || pipe(pipeToShell) < 0)
    {
        fprintf(stderr, "Pipe failed: %s,", strerror(errno));
        exit(1);
    }

    //Register signal handler for SIGPIPE
    if (signal(SIGPIPE, signal_handler) == SIG_ERR)
    {
        fprintf(stderr, "Signal registering failed: %s", strerror(errno));
        exit(1);
    }

    //Fork the program
    processID = fork();

    switch (processID)
    {
    case -1:
        fprintf(stderr, "Unable to fork successfully: %s", strerror(errno));
        exit(1);
        break;

    case 0:
        //Child process
        //Duplicate necessary pollArray to redirect stdin, stdout, stderr to and from pipes
        //Close terminal end of pipes
        safeClose(pipeToShell[WRITE_END]);
        safeClose(pipeFromShell[READ_END]);
        safeDup2(pipeToShell[READ_END], STDIN_FILENO);
        safeDup2(pipeFromShell[WRITE_END], STDOUT_FILENO);
        safeDup2(pipeFromShell[WRITE_END], STDERR_FILENO);
        safeClose(pipeToShell[READ_END]);
        safeClose(pipeFromShell[WRITE_END]);

        char *const execOptions[1] = {NULL};

        if (execvp("/bin/bash", execOptions) < 0)
        {
            fprintf(stderr, "Unable to start shell: %s", strerror(errno));
            exit(1);
        }

        if (debugFlag)
            printf("Child process started, shell started\n");
        break;

    default:
        //Parent Process
        //Close shell end of pipes
        safeClose(pipeToShell[READ_END]);
        safeClose(pipeFromShell[WRITE_END]);

        //Setup polling
        //Keyboard
        serverPollArray[KEYBOARD].fd = connectedSocketfd;
        serverPollArray[KEYBOARD].events = POLLIN;

        //Output from shell
        serverPollArray[SHELL].fd = pipeFromShell[READ_END];
        serverPollArray[SHELL].events = POLL_IN | POLL_HUP | POLL_ERR;

        //Set up meta
        serverMeta[FORMAT] = DEFAULT;
        serverMeta[PIPE_TO_SHELL_READ] = pipeToShell[READ_END];
        serverMeta[PIPE_TO_SHELL_WRITE] = pipeToShell[WRITE_END];
        serverMeta[PIPE_FROM_SHELL_READ] = pipeFromShell[READ_END];
        serverMeta[PIPE_FROM_SHELL_WRITE] = pipeFromShell[WRITE_END];
        serverMeta[PROC_ID] = processID;
        serverMeta[SOCKET] = connectedSocketfd;
        serverMeta[SENDER] = SERVER;
        serverMeta[LOG] = 0;

        //Hand over to Utility Function readOrPoll
        readOrPoll(serverPollArray, readBuffer, serverMeta, stderr);
        break;
    }

    exit(0);
}
