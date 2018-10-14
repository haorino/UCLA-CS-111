#ifndef UTILITY_FUNCTIONS
#define UTILITY_FUNCTIONS

#include <stdio.h>
#include <poll.h>

// Writes numBytes worth of data from a given buffer
// Deals with special character according to format - specified in meta
void writeBytes(int numBytes, int writeFD, char *buffer, int *meta);

// Polls pipeFromShell and pipeToShell 's read ends for input and interrupts
void readOrPoll(struct pollfd *pollArray, char *readBuffer, int *meta, FILE *logFile);

#endif