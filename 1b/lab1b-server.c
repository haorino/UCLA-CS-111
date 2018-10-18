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
#include <mcrypt.h>
#include <sys/stat.h>
//Custom libraries and header files
#include "safeSysCalls.h"
#include "Constants.h"

// Global Variables
char readBuffer[BUFFERSIZE];
int portFlag, encryptFlag, debugFlag;
int portNum;
int pipeToShell[2], pipeFromShell[2];
int initSocketfd, connectedSocketfd;
unsigned int clientAddressLength;
int processID;
struct pollfd serverPollArray[2];
struct sockaddr_in serverAddress, cllientAddress;
MCRYPT cipher, decipher;
char *encryptionKey;
int keyLength;
struct stat keyStat;


/* --- Signal Handler --- */
void signal_handler(int sig)
{
    if (sig == SIGPIPE)
    {
        exit(0);
    }
}

/* --- Encyrption functions --- */
void encrypt(char *buffer, int cryptLength)
{
    if (mcrypt_generic(cipher, buffer, cryptLength) != 0)
    {
        fprintf(stderr, "Encryption error: %s", strerror(errno));
        exit(1);
    }
}

void decrypt(char *buffer, int decryptLength)
{

    if (mcrypt_generic(cipher, buffer, decryptLength) != 0)
    {
        fprintf(stderr, "Decryption error: %s", strerror(errno));
        exit(1);
    }
}

void setupEncryption(char *key, int keyLength)
{
    cipher = mcrypt_module_open("blowfish", NULL, "ofb", NULL);
    if (cipher == MCRYPT_FAILED)
    {
        fprintf(stderr, "Module opening error: %s", strerror(errno));
        exit(1);
    }
    if (mcrypt_generic_init(cipher, key, keyLength, NULL) < 0)
    {
        fprintf(stderr, "MCRYPT initialization error: %s", strerror(errno));
        exit(1);
    }

    decipher = mcrypt_module_open("blowfish", NULL, "ofb", NULL);
    if (decipher == MCRYPT_FAILED)
    {
        fprintf(stderr, "Module opening error: %s", strerror(errno));
        exit(1);
    }
    if (mcrypt_generic_init(decipher, key, keyLength, NULL) < 0)
    {
        fprintf(stderr, "Decryption error: %s", strerror(errno));
        exit(1);
    }
}

void encryptionShutDown()
{
    mcrypt_generic_deinit(cipher);
    mcrypt_module_close(cipher);

    mcrypt_generic_deinit(decipher);
    mcrypt_module_close(decipher);
}
/* --- Utility Functions --- */
// Restores to pre-execution environment: frees memory, closes pipes etc.
void restore()
{
    //Free Memory
    free(encryptionKey);

    //De-initialize encryption tools
    if (encryptFlag > 0)
        encryptionShutDown();

    //waitpid and shell exit code harvesting
    int status;
    if (waitpid(processID, &status, 0) == -1)
        fprintf(stderr, "WAITPID failed: %s\n", strerror(errno));
    fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", WTERMSIG(status), WEXITSTATUS(status));
}

/* --- Primary Functions --- */

// Writes numBytes worth of data from a given buffer
// Deals with special character according to format - specified in meta
void writeBytes(int numBytes, int writeFD, char *buffer)
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
            if (writeFD == pipeToShell[WRITE_END])
                safeWrite(writeFD, "\n", 1);
            else
                safeWrite(writeFD, charPosition, 1);
            break;
        //C-d
        case 4:
            if (writeFD == pipeToShell[WRITE_END])
            {
                safeClose(pipeToShell[WRITE_END]);
                breakFlag = 1;
            }
            else
                safeWrite(writeFD, charPosition, 1);
            break;
        //C-c
        case 3:
            if (writeFD == pipeToShell[WRITE_END])
            {
                safeKill(processID, SIGINT);
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
            fprintf(stderr, "Polling error: %s\n", strerror(errno));
            exit(1);
        }
        //Nothing to poll - no reads needed go ahead, to next iteration
        if (pendingReads == 0)
            continue;

        //.revents --> returns bits of events that occured, hence using bit manipulation...
        //Keyboard has data to be read
        if (pollArray[KEYBOARD].revents & POLLIN)
        {

            //Data has been sent in from client, need to read it
            numBytes = safeRead(connectedSocketfd, readBuffer, BUFFERSIZE);

            //Decrypt
            //Decrypt Buffer
            if (encryptFlag > 0)
            {
                int dcount;
                for (dcount = 0; dcount < numBytes; dcount++)
                    decrypt(readBuffer + dcount, 1);
            }

            //Data received from client, must be forwarded to shell
            writeBytes(numBytes, pipeToShell[WRITE_END], readBuffer);
        }

        //Shell has output to be reads
        if (pollArray[SHELL].revents & POLLIN)
        {

            // Shell has output to be read
            numBytes = safeRead(pipeFromShell[READ_END], readBuffer, BUFFERSIZE);

            //Encrypt Buffer
            if (encryptFlag > 0)
            {
                int eCount;
                for (eCount = 0; eCount < numBytes; eCount++)
                    encrypt(readBuffer + eCount, 1);
            }

            //Data has been sent over from actual shell, need to write to socket to client
            writeBytes(numBytes, connectedSocketfd, readBuffer);
        }

        if (pollArray[SHELL].revents & (POLLHUP | POLLERR))
            exit(1);
    }
    return;
}

int main(int argc, char *argv[])
{
    //Options

    int option = 1;
    encryptFlag = 0;
    portFlag = 0;

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
            encryptFlag = open(optarg, O_RDONLY);
            if (encryptFlag < 0)
            {
                fprintf(stderr, "Opening error: %s\n", strerror(errno));
                exit(1);
            }
            else
            {
                if (fstat(encryptFlag, &keyStat) < 0)
                {
                    fprintf(stderr, "Fstat error: %s", strerror(errno));
                    exit(1);
                }
                if ((encryptionKey = malloc(keyStat.st_size * sizeof(char))) < 0)
                {
                    fprintf(stderr, "Memory allocation error: %s", strerror(errno));
                    exit(1);
                }
                keyLength = safeRead(encryptFlag, encryptionKey, keyStat.st_size * sizeof(char));
            }
            break;
        default:
            fprintf(stderr, "Usage: %s --port=portNum [--encrypt=file.key]\n", argv[0]);
            exit(1);
        }
    }

    //Check for port flag, else abort with usage message
    if (!portFlag)
    {
        fprintf(stderr, "Port option is mandatory. Usage: %s --port=portNum [--encrypt=file.key]\n", argv[0]);
        exit(1);
    }

    atexit(restore);

    //Set Up Socket to communicate with client process
    initSocketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (initSocketfd < 0)
    {
        fprintf(stderr, "Socket creation failed: %s\n", strerror(errno));
        exit(1);
    }

    bzero((char *)&serverAddress, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(portNum);

    if (bind(initSocketfd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        fprintf(stderr, "Socket binding error: %s\n", strerror(errno));
        exit(1);
    }

    listen(initSocketfd, 5);
    clientAddressLength = sizeof(cllientAddress);
    connectedSocketfd = accept(initSocketfd, (struct sockaddr *)&cllientAddress, &clientAddressLength);

    if (connectedSocketfd < 0)
    {
        fprintf(stderr, "Error while accepting connection to socket: %s\n", strerror(errno));
        exit(1);
    }

    //Zero out buffer
    bzero(readBuffer, BUFFERSIZE);

    //Make non-blocking socket for I/O
    int flags = fcntl(0, F_GETFL);
    fcntl(connectedSocketfd, F_SETFL, flags | O_NONBLOCK);

    //Set up pipes to communicate between the shell and the new terminal
    if (pipe(pipeFromShell) < 0 || pipe(pipeToShell) < 0)
    {
        fprintf(stderr, "Pipe failed: %s,\n", strerror(errno));
        exit(1);
    }

    //Register signal handler for SIGPIPE
    if (signal(SIGPIPE, signal_handler) == SIG_ERR)
    {
        fprintf(stderr, "Signal registering failed: %s\n", strerror(errno));
        exit(1);
    }

    //Fork the program
    processID = fork();

    switch (processID)
    {
    case -1:
        fprintf(stderr, "Unable to fork successfully: %s\n", strerror(errno));
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
            fprintf(stderr, "Unable to start shell: %s\n", strerror(errno));
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

        if (encryptFlag > 0)
            setupEncryption(encryptionKey, keyLength);

        //Hand over to Utility Function readOrPoll
        readOrPoll(serverPollArray, readBuffer);
        break;
    }

    exit(0);
}
