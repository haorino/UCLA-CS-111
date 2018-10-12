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

//Custom libraries and header files
#include "safeSysCalls.h"
#include "Constants.h"

// Variables
struct termios defaultMode;
char *readBuffer;
char *writeBuffer;
int portFlag;
int encryptFlag;
int debugFlag;
int portNum;
struct pollfd pollArray[2];
int pipeToShell[2];
int pipeFromShell[2];

int processID;

/* --- Utility Functions --- */
// Restores to pre-execution environment: frees memory, resets terminal attributes, closes pipes etc.
void restore()
{
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
            portNum = optarg;
            break;
        case 'e':
            debugFlag = 1;
            break;
        default:
            fprintf(stderr, "Usage: %s [--shell]", argv[0]);
            exit(1);
        }
    }

    atexit(restore);

    //Allocate memory to readBuffer
    readBuffer = (char *)malloc(sizeof(char) * BUFFERSIZE);

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
        pollArray[KEYBOARD].fd = STDIN_FILENO;
        pollArray[KEYBOARD].events = POLLIN;

        //Output from shell
        pollArray[SHELL].fd = pipeFromShell[READ_END];
        pollArray[SHELL].events = POLL_IN | POLL_HUP | POLL_ERR;

        readOrPoll();
        break;
    }

    exit(0);
}
