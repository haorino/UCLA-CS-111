#include <stdio.h>
#include <errno.h>
#include "safeSysCalls.h"

/* --- System calls with error handling --- */
int safeRead(int fd, char *buffer, ssize_t size)
{
    int status = read(fd, buffer, size);
    if (status < 0)
    {
        fprintf(stderr, "Unable to read from STDIO %s", strerror(errno));
        exit(1);
    }

    return status;
}

int safeWrite(int fd, char *buffer, ssize_t size)
{
    int status = write(fd, buffer, size);
    if (status != size)
    {
        fprintf(stderr, "Unable to write to STDOUT %s", strerror(errno));
        exit(1);
    }

    return status;
}

void safeDup2(int original_fd, int new_fd)
{
    if (dup2(original_fd, new_fd) < 0)
    {
        fprintf(stderr, "Dup2 failed: %s", strerror(errno));
        exit(1);
    }
}

void safeClose(int fd)
{
    if (close(fd) < 0)
    {
        fprintf(stderr, "Error closing file: %s", strerror(errno));
        exit(1);
    }
}

void safeKill(processID, SIGINT)
{
    if (kill(processID, SIGINT) < 0)
    {
        fprintf(stderr, "Kill failed: %s", strerror(errno));
        exit(1);
    }
}