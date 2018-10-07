#include <termios.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <getopt.h>
#include <pthread.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

//Globals
#define BUFFERSIZE 256
struct termios defaultMode;
char* readBuffer;
char* writeBuffer;
ssize_t writeSize;
int shell_flag;
int fromShell_fd[2];
int toShell_fd[2];

void restore ()
{
    //Free memory
    free(readBuffer);
    //Reset the modes
    tcsetattr(0, TCSANOW, &defaultMode);
}

//Error handling done alongside sys call
int readCorrect ()
{
    int status = read(STDIN_FILENO, readBuffer, BUFFERSIZE);
    if (status < 0)
    {
        fprintf(STDERR_FILENO, "Unable to read from STDIO %s", strerr(errno));
        restore(readBuffer, &defaultMode);
        exit(1);
    }

    return status;
}

//Error handling done alongside sys call
int writeCorrect ()
{
    int status = write(STDOUT_FILENO, writeBuffer, writeSize);
    if (status != writeSize)
    {
        fprintf(STDERR_FILENO, "Unable to write to STDOUT %s", strerr(errno));
        ;
        exit(1);
    }

    return status;
}

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
    tcsetattr(0, TCSANOW, &projectMode); //TCSANOW opt does the action immediately
    atexit(restore);

    //Set up pipes to communicate between the shell and the new terminal
    //Fork the program
    //Set up polling function
    if (shell_flag)
    {
        create_pipe(fromShell_fd);
        create_pipe(toShell_fd);
    }

    //Allocate memory to readBuffer
    readBuffer = (char*) malloc(sizeof(char) * BUFFERSIZE);

    //Open stdinput for reading
    int stdio_success = open(STDIN_FILENO, O_RDONLY);
    
    //If unable to input stdio, exit with error
    if (stdio_success < 0)
    {
        fprintf(STDERR_FILENO, "Unable to open STDIO %s", strerr(errno));
        exit(1);
    }

    while (1)
    {
        int numBytes = readCorrect();
        int displacement;
        for (displacement = 0; displacement < numBytes; displacement++)
        {
            switch (*(readBuffer + displacement))
            {
                case 4:
                //EOF detected
                    exit(0);
                    break;
                
                case '\r':
                case '\n':
                    writeBuffer = "\r\n";
                    writeSize = 2;
                    break;

                default:
                    writeBuffer = readBuffer + displacement;
                    writeSize = 1;
                    break; 
            }
            writeCorrect();
        }
    }
    exit(0);
}