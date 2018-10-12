#ifndef SAFE_SYS_CALLS
#define SAFE_SYS_CALLS

//Safe System Calls Library Header File
#include <stdio.h>
/* --- System calls with error handling --- */

int safeRead (int fd, char* buffer, ssize_t size);

int safeWrite (int fd, char* buffer, ssize_t size);

void safeDup2(int original_fd, int new_fd);

void safeClose(int fd);

void safeKill(int processID, int SIGNUM);

#endif