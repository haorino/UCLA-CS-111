#include <termios.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>

/* --- Global Variables --- */
#define BUFFERSIZE 256
struct termios defaultMode;
char* readBuffer;
char* writeBuffer;
int shell_flag;
int fromShell_fd[2];
int toShell_fd[2];
//poll_fds[0] --> keyboard | poll_fds[1] --> output from shell
struct pollfd poll_fds[2]; 
int processID;

/* --- System calls with error handling --- */
int safeRead (int fd, char* buffer)
{
    int status = read(fd, buffer, BUFFERSIZE);
    if (status < 0)
    {
        fprintf(STDERR_FILENO, "Unable to read from STDIO %s", strerr(errno));
        exit(1);
    }

    return status;
}

int safeWrite (int fd, char* buffer, ssize_t size)
{
    int status = write(fd, buffer, size);
    if (status != size)
    {
        fprintf(STDERR_FILENO, "Unable to write to STDOUT %s", strerr(errno)); 
        exit(1);
    }

    return status;
}

/* --- Utility Functions --- */
//Restores to pre-execution environment: frees memory, resets terminal attributes, closes pipes etc.
void restore ()
{
    //Free memory
    free(readBuffer);

    //Reset the modes
    tcsetattr(0, TCSANOW, &defaultMode);
}

//Writes numBytes worth of data from a given buffer: ^D exits, CR or LF -> CRLF
void writeBytes(int numBytes, int writeFD, char* buffer)
{
    int displacement;

    for (displacement = 0; displacement < numBytes; displacement++)
    {

        int writeSize;
        switch (*(buffer + displacement))
        {
            case 4:
                //EOF detected
                exit(0);
                break;
                    
            case '\r':
            case '\n':
                buffer = "\r\n";
                writeSize = 2;
                break;

            default:
                writeBuffer = readBuffer + displacement;
                writeSize = 1;
                break; 
        }

        //Echos output to stdout or writes to pipe
        safeWrite(writeFD, buffer, writeSize);
        //Forwards output to shell
        if (shell_flag && processID)
            writeCorrect(toShell_fd[1]);
    }
}

//Polls fromShell and toShell 's read ends for input and interrupts
void readOrPoll()
{
    //Infinite loop to keep reading and/or polling
    while (1)
    {
        int pendingReads = poll(poll_fds, 2, 0);
        int numBytes = 0;
        
        //Polling sys call failure
        if (pendingReads < 0)
        {
            fprintf(stderr, "Polling error: ", strerr(errno));
            exit(1);
        }

        //Successful polling
        else 
        {
            //Nothing to poll - no reads needed go ahead, to next iteration
            if (pendingReads == 0)
                continue;

            //.revents --> returns bits of events that occured, hence using bit manipulation...
            if (poll_fds[0].revents & POLLIN)
            {
                //Data has been sent in from keyboard, need to read it
                numBytes = safeRead(STDIN_FILENO, readBuffer);
                writeBytes(numBytes, STDOUT_FILENO, readBuffer);
            }  

            if (poll_fds[1].revents & POLLIN)
            {
                numBytes = safeRead(fromShell_fd[1], readBuffer);
                writeBytes(numBytes, STDOUT_FILENO, readBuffer);
            }

            if (poll_fds[1].revents & (POLLHUP | POLLERR))
            {
                fprintf(stderr, "Shell terminated.");
                exit(1);
            }
        }
    }
    return;
}

/* --- Main Function --- */

int main(int argc, char* argv[])
{
    //Options
    shell_flag = 0;
    int option = 1;
    static struct option shell_options [] = {
        {"shell", optional_argument, 0, 's'}
    };

    while ((option = getopt_long(argc, argv, "s", shell_options, NULL)) > -1)
    {
        switch (option)
        {
            case 's':
                shell_flag = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [--shell=program]", argv[0]);
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
        fprintf(stderr, "Unable to set non-canonical input mode with no echo: %s", strerr(errno));
        exit(1);   
    }

    //Allocate memory to readBuffer
    readBuffer = (char*) malloc(sizeof(char) * BUFFERSIZE);

    
    if (shell_flag)
    {
        //Set up pipes to communicate between the shell and the new terminal
        create_pipe(fromShell_fd);
        create_pipe(toShell_fd);

        //Fork the program
        processID = fork();
        switch (processID)
        {
            case -1:
                fprintf(stderr, "Unable to fork successfully: %s", strerr(errno));
                exit(1);
                break;
            case 0:
            //Child process
            //Duplicate necessary poll_fds to redirect stdin, stdout, stderr to and from pipes
            //Close terminal end of pipes
                close(fromShell_fd[0]);
                close(toShell_fd[1]);
                dup2(toShell_fd[0],   STDIN_FILENO);
                dup2(fromShell_fd[1], STDOUT_FILENO);
                dup2(fromShell_fd[1], STDERR_FILENO);
                execlp("/bin/bash", "sh", NULL);
                break;
            default:
            //Parent Process
            //Setup polling
                //Keyboard
                poll_fds[0].fd = STDIN_FILENO;
                poll_fds[0].events = POLLIN;

                //Output from shell
                poll_fds[1].fd = fromShell_fd[0];
                poll_fds[1].events = POLL_IN | POLL_HUP | POLL_ERR;
                break;
        }

        readOrPoll();
    }

    else
    {
        while(1)
        {
            int numBytes = safeRead(STDIN_FILENO, readBuffer);
            writeBytes(numBytes, STDOUT_FILENO, readBuffer);
        }
    }

    exit(0);
}