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

//Custom libraries and header files
#include "safeSysCalls.h"
#include "Constants.h"

// Variables
struct termios defaultMode;
char *readBuffer;
char *writeBuffer;
int port_flag;
int portNum;
int encrypt_flag;
struct pollfd clientPollArray[2];
int processID;

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
        {"encrypt", no_argument, 0, 'e'}};

    while ((option = getopt_long(argc, argv, "p:e", shell_options, NULL)) > -1)
    {
        switch (option)
        {
        case 'p':
            port_flag = 1;
            portNum = optarg;
            break;
        case 'e':
            encrypt_flag = 1;
            break;

        default:
            fprintf(stderr, "Usage: %s [--shell]", argv[0]);
            exit(1);
        }
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

    //Set up socket to communicate between the client and the server

    clientPollArray[KEYBOARD].fd = STDIN_FILENO;
    clientPollArray[KEYBOARD].events = POLLIN;

    //Output from shell (coming from server)
    // clientPollArray[SHELL].fd = socket
    clientPollArray[SHELL].events = POLL_IN | POLL_HUP | POLL_ERR;

    readOrPoll();

    exit(0);
}
