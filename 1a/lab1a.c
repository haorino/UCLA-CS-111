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

//Globals
#define BUFFERSIZE 256
struct termios defaultMode;
char* readBuffer;
char* writeBuffer;
ssize_t writeSize;
int shell_flag;
int fromShell_fd[2];
int toShell_fd[2];
struct pollfd keyboard ;
struct pollfd shell_output;

int timeout;
int process_id;

//Restore - by freeing memory, closing pipes if shell_flag and resetting terminal attributes
void restore ()
{
    //Free memory
    free(readBuffer);
    //Reset the modes
    tcsetattr(0, TCSANOW, &defaultMode);
}


//Error handling done alongside sys call
//as well as implementing additionally functionality of writing to pipes for parent process
int readCorrect (int fd)
{
    int status = read(fd, readBuffer, BUFFERSIZE);
    if (status < 0)
    {
        fprintf(STDERR_FILENO, "Unable to read from STDIO %s", strerr(errno));
        exit(1);
    }

    return status;
}

//Error handling done alongside sys call, as well as implementing
int writeCorrect (int fd)
{
    int status = write(fd, writeBuffer, writeSize);
    if (status != writeSize)
    {
        fprintf(STDERR_FILENO, "Unable to write to STDOUT %s", strerr(errno)); 
        exit(1);
    }

    return status;
}

void readWrite()
{
    //Allocate memory to readBuffer
    readBuffer = (char*) malloc(sizeof(char) * BUFFERSIZE);

    //Infinite loop to keep reading and writing
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

            //echos output to stdout, or writes it to pipe
            writeCorrect(STDOUT_FILENO);

            //forwards output to shell
            if (shell_flag && process_id)
                writeCorrect(toShell_fd[1]);
        }
    }
    return;
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
    //TCSANOW opt does the action immediately
    if (tcsetattr(0, TCSANOW, &projectMode) < 0)
    {
        fprintf(stderr, "Unable to set non-canonical input mode with no echo: %s", strerr(errno));
        exit(1);   
    }

    //Set up pipes to communicate between the shell and the new terminal
    //Fork the program
    //Set up polling function
    if (shell_flag)
    {
        create_pipe(fromShell_fd);
        create_pipe(toShell_fd);

        process_id = fork();
        switch (process_id)
        {
            case -1:
                fprintf(stderr, "Unable to set non-canonical input mode with no echo: %s", strerr(errno));
                exit(1);
                break;
            case 0:
            //Child process
            //Duplicate necessary fds to redirect stdin, stdout, stderr to and from pipes
            //Close terminal end of pipes
                close(fromShell_fd[0]);
                close(toShell_fd[1]);
                dup2(toShell_fd[0],   STDIN_FILENO);
                dup2(fromShell_fd[1], STDOUT_FILENO);
                dup2(fromShell_fd[1], STDERR_FILENO);
                execlp("/bin/bash", NULL);
                break;
            default:
            //Parent Process
            //Setup polling
                break;
        }
    }

    readWrite();
    exit(0);
}