#include "UtilityFunctions.h"
#include "SafeSysCalls.h"
#include "Constants.h"

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <stdlib.h>
#include <mcrypt.h>

MCRYPT cipherIn, cipherOut, decipherIn, decipherOut;
int keyfd;

void setUpEncryptionDecryption(MCRYPT* obj)
{
    obj = mcrypt_module_open("twofish", NULL, "cfb", NULL); 
    if(obj == MCRYPT_FAILED)
    {
        fprintf(stderr, "Module opening failed: %s\n", strerror(errno));
        exit(1);
    }

    char* encryptionKey;
    encryptionKey = malloc(KEY_LENGTH); 
    if(encryptionKey == NULL)
    {
        fprintf(stderr, "Memory allocation error: %s\n", strerror(errno));
        exit(1);
    }

    int initVectorSize = mcrypt_enc_get_iv_size(obj);
    int* initVector;
    initVector = malloc(initVectorSize);
    if(initVector == NULL) 
    {
        fprintf(stderr, "Memory allocation error: %s\n", strerror(errno));
        exit(1);
    }

    memset(encryptionKey, 0, KEY_LENGTH);
    safeRead(keyfd, encryptionKey, KEY_LENGTH);
    safeClose(keyfd);

    int i;
    for(i = 0; i < initVectorSize; i++)
        initVector[i] = 0; 

    if(mcrypt_generic_init(obj, encryptionKey, KEY_LENGTH, initVector) < 0)
    {
        fprintf(stderr, "Error initializing encryption module: %s\n", sterror(errno));
        exit(1);
    }

    free(encryptionKey);
    free(initVector);
}

// Writes numBytes worth of data from a given buffer
// Deals with special character according to format - specified in meta
void writeBytes(int numBytes, int writeFD, char *buffer, int *meta)
{
    int displacement;
    int breakFlag = 0;
    for (displacement = 0; displacement < numBytes; displacement++)
    {
        char *charPosition = buffer + displacement;
        switch (*(charPosition))
        {
        case '\r':
        case '\n':
            if (meta[FORMAT] == STDOUT_FORMAT)
                safeWrite(writeFD, "\r\n", 2);
            else if (meta[FORMAT] == SHELL_FORMAT)
                safeWrite(writeFD, "\n", 1);
            else
                safeWrite(writeFD, charPosition, 1);
            break;
        //C-d
        case 4:
            if (meta[FORMAT] == SHELL_FORMAT)
            {
                safeClose(meta[PIPE_TO_SHELL_WRITE]);
                breakFlag = 1;
            }
            else
                safeWrite(writeFD, charPosition, 1);
            break;
        //C-c
        case 3:
            if (meta[FORMAT] == SHELL_FORMAT)
            {
                safeKill(meta[PROC_ID], SIGINT);
                breakFlag = 1;
            }
            else
                safeWrite(writeFD, charPosition, 1);
            break;
        default:
            safeWrite(writeFD, charPosition, 1);
            break;
        }

        if (breakFlag)
            break;
    }
}

// Polls pipeFromShell and pipeToShell 's read ends for input and interrupts
void readOrPoll(struct pollfd *pollArray, char *readBuffer, int *meta)
{
    setUpEncryptionDecryption(cipherIn);
    setUpEncryptionDecryption(cipherOut);
    setUpEncryptionDecryption(decipherIn);
    setUpEncryptionDecryption(decipherOut);
    
    keyfd = meta[KEY_FD];
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
                if (meta[SENDER] == CLIENT)
                {
                    //Data has been sent in from keyboard, need to read it
                    numBytes = safeRead(STDIN_FILENO, readBuffer, BUFFERSIZE);

                    meta[FORMAT] = STDOUT_FORMAT;
                    //Data in from actual keyboard, must be echoed to screen
                    writeBytes(numBytes, STDOUT_FILENO, readBuffer, meta);

                    if (meta[LOG])
                        fprintf(meta[LOG], "SENT %d bytes: %s", numBytes, readBuffer);

                    //Encrypt
                    meta[FORMAT] = DEFAULT; // Reset format for writing to socket
                    //Data must be written to socket to communicate
                    writeBytes(numBytes, meta[SOCKET], readBuffer, meta);
                }

                else if (meta[SENDER] == SERVER)
                {
                    //Data has been sent in from client, need to read it
                    numBytes = safeRead(meta[SOCKET], readBuffer, BUFFERSIZE);

                    //Decrypt
                    
                    meta[FORMAT] = SHELL_FORMAT;
                    //Data received from client, must be forwarded to shell
                    writeBytes(numBytes, meta[PIPE_TO_SHELL_WRITE], readBuffer, meta);
                }

                else
                    printf("Aborting... - sender not specified");
            }

            //Shell has output to be reads
            if (pollArray[SHELL].revents & POLLIN)
            {
                
                if (meta[LOG])
                    fprintf(meta[LOG], "RECEIVED %d bytes: %s", numBytes, readBuffer);

                if (meta[SENDER] == CLIENT)
                {
                    // Server has output to be read
                    numBytes = safeRead(meta[SOCKET], readBuffer, BUFFERSIZE);

                    //Decrypt Buffer

                    meta[FORMAT] = STDOUT_FORMAT;
                    //Data has been sent over from server, need to display to screen
                    writeBytes(numBytes, STDOUT_FILENO, readBuffer, meta);
                }

                else if (meta[SENDER] == SERVER)
                {
                    // Shell has output to be read
                    numBytes = safeRead(meta[PIPE_FROM_SHELL_READ], readBuffer, BUFFERSIZE);

                    //Encrypt

                    meta[FORMAT] = DEFAULT;
                    //Data has been sent over from actual shell, need to write to socket to client
                    writeBytes(numBytes, meta[SOCKET], readBuffer, meta);
                }
                else
                    printf("Aborting... - sender not specified");
            }

            if (pollArray[SHELL].revents & (POLLHUP | POLLERR))
                exit(1);
        }
    }
    return;
}