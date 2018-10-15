#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include "safeSysCalls.h"

/* --- System calls with error handling --- */
int safeRead(int fd, char *buffer, ssize_t size)
{
    int status = read(fd, buffer, size);
    if (status < 0)
    {
        fprintf(stderr, "Unable to read from input %s\n", strerror(errno));
        exit(1);
    }

    return status;
}

int safeWrite(int fd, char *buffer, ssize_t size)
{
    int status = write(fd, buffer, size);
    if (status != size)
    {
        fprintf(stderr, "Unable to write to STDOUT %s\n", strerror(errno));
        exit(1);
    }

    return status;
}

void safeDup2(int original_fd, int new_fd)
{
    if (dup2(original_fd, new_fd) < 0)
    {
        fprintf(stderr, "Dup2 failed: %s\n", strerror(errno));
        exit(1);
    }
}

void safeClose(int fd)
{
    if (close(fd) < 0)
    {
        fprintf(stderr, "Error closing file: %s\n", strerror(errno));
        exit(1);
    }
}

void safeKill(int processID, int SIGNUM)
{
    if (kill(processID, SIGNUM) < 0)
    {
        fprintf(stderr, "Kill failed: %s\n", strerror(errno));
        exit(1);
    }
}
