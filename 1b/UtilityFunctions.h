#ifndef UTILITY_FUNCTIONS
#define UTILITY_FUNCTIONS

#include "SafeSysCalls.h"
#include "Constants.h"

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <stdlib.h>

// Writes numBytes worth of data from a given buffer: ^D exits, CR or LF -> CRLF
void writeBytes(int numBytes, int writeFD, char *buffer)
{
    int displacement;

    for (displacement = 0; displacement < numBytes; displacement++)
    {

        switch (*(buffer + displacement))
        {
        case '\r':
        case '\n':
            if (writeFD == STDOUT_FILENO)
                safeWrite(writeFD, "\r\n", 2);
            else
                safeWrite(writeFD, buffer + displacement, 1);
            break;

        default:
            safeWrite(writeFD, buffer + displacement, 1);
            break;
        }
    }
}

// Writes numBytes worth of data from a given buffer: ^D exits, CR or LF -> CRLF
void writeBytes(int numBytes, int writeFD, char *buffer)
{
    int displacement;

    for (displacement = 0; displacement < numBytes; displacement++)
    {

        switch (*(buffer + displacement))
        {
        case 4:
            //EOF detected
            //safeClose(pipeToShell[1]);

            break;

        case 3:

        {
            //safeKill(processID, SIGINT);
        }
        break;

        case '\r':
        case '\n':
            /* if (writeFD == pipeToShell[WRITE_END])
                safeWrite(writeFD, "\n", 1);
            else
                safeWrite(writeFD, "\r\n", 2);
            break;
            */
        default:
            safeWrite(writeFD, buffer + displacement, 1);
            break;
        }
    }
}

// Polls pipeFromShell and pipeToShell 's read ends for input and interrupts
void readOrPoll(struct pollfd *pollArray, char *readBuffer)
{
    //Infinite loop to keep reading and/or polling
    while (1)
    {
        int pendingReads = poll(pollArray, 2, 0);
        int numBytes = 0;

        //Polling sys call failure
        if (pendingReads < 0)
        {
            fprintf(stderr, "Polling error: %s", strerror(errno));
            exit(1);
        }

        //Successful polling
        else
        {
            //Nothing to poll - no reads needed go ahead, to next iteration
            if (pendingReads == 0)
                continue;

            //.revents --> returns bits of events that occured, hence using bit manipulation...
            //Keyboard has data to be read
            if (pollArray[KEYBOARD].revents & POLLIN)
            {
                //Data has been sent in from keyboard, need to read it
                numBytes = safeRead(STDIN_FILENO, readBuffer, BUFFERSIZE);
                writeBytes(numBytes, STDOUT_FILENO, readBuffer);
                //  writeBytes(numBytes, pipeToShell[WRITE_END], readBuffer);
            }

            //Shell has output that must be read
            /* if (pollArray[SHELL].revents & POLLIN)
            {
                numBytes = safeRead(pipeFromShell[READ_END], readBuffer);
                writeBytes(numBytes, STDOUT_FILENO, readBuffer);
            }
            */
            if (pollArray[SHELL].revents & (POLLHUP | POLLERR))
                exit(1);
        }
    }
    return;
}



#endif