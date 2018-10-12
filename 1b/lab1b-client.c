#include <string.h>
#include <termios.h>
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

/* --- Globals  --- */
// Constants
#define BUFFERSIZE 256
#define WRITE_END 1
#define READ_END 0
#define KEYBOARD 0
#define SHELL 1

// Variables
struct termios defaultMode;
char* readBuffer;
char* writeBuffer;
char* customShell;
int shell_flag;
int debug_flag;
int pipeFromShell[2];
int pipeToShell[2];
struct pollfd pollArray[2]; 
int processID;



/* --- Utility Functions --- */
// Restores to pre-execution environment: frees memory, resets terminal attributes, closes pipes etc.
void restore ()
{
    //Free memory
    free(readBuffer);
    
    //Reset the modes
    tcsetattr(0, TCSANOW, &defaultMode);

    //If shell option selected, wait for shell to close and capture the exit code
    if (shell_flag)
    {
        int status;
        if (waitpid(processID, &status, 0) == -1)
        {
            fprintf(stderr, "waitpid failed: %s", strerror(errno));
        }
        fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", WTERMSIG(status), WEXITSTATUS(status));

    }
}

// Writes numBytes worth of data from a given buffer: ^D exits, CR or LF -> CRLF
void writeBytes(int numBytes, int writeFD, char* buffer)
{
    int displacement;

    for (displacement = 0; displacement < numBytes; displacement++)
    {

        switch (*(buffer + displacement))
        {
            case 4:
                //EOF detected
                if (shell_flag)
                    close(pipeToShell[1]);
                else
                    exit(0);
                break;

            case 3:
                if (shell_flag)
		{
		   if (kill (processID, SIGINT) < 0)
		   {
		       fprintf(stderr, "Kill failed: %s", strerror(errno));
		       exit(1);
		   }
		 }
                break;

            case '\r':
            case '\n':
                if (writeFD == pipeToShell[WRITE_END])
		  safeWrite(writeFD, "\n", 1);
                else
		  safeWrite(writeFD, "\r\n", 2);
                  break;

            default:
	        safeWrite(writeFD, readBuffer + displacement, 1);
                break; 
        }
    }
}

// Polls pipeFromShell and pipeToShell 's read ends for input and interrupts
void readOrPoll()
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
                numBytes = safeRead(STDIN_FILENO, readBuffer);
                writeBytes(numBytes, STDOUT_FILENO, readBuffer);
		writeBytes(numBytes, pipeToShell[WRITE_END], readBuffer);
            }  
	    
	    //Shell has output that must be read
            if (pollArray[SHELL].revents & POLLIN)
            {
                numBytes = safeRead(pipeFromShell[READ_END], readBuffer);
                writeBytes(numBytes, STDOUT_FILENO, readBuffer);
            }

            if (pollArray[SHELL].revents & (POLLHUP | POLLERR))
            {
                if (debug_flag)
                    fprintf(stderr, "Shell terminated.\n");
                exit(1);
            }
        }
    }
    return;
}

/* --- Signal Handler --- */
void signal_handler(int sig)
{
    if (sig == SIGPIPE)
    {
        exit(0);
    }
}

/* --- Main Function --- */

int main(int argc, char* argv[])
{
    //Options
    shell_flag = 0;
    debug_flag = 0;
    int option = 1;

    static struct option shell_options [] = {
      {"shell", optional_argument, 0, 's'},
      {"debug", no_argument, 0, 'd'}
    };

    while ((option = getopt_long(argc, argv, "s", shell_options, NULL)) > -1)
    {
        switch (option)
        {
            case 's':
                shell_flag = 1;
                customShell = optarg;
                break;
	    case 'd':
	      debug_flag = 1;
	      break;	  
            default:
                fprintf(stderr, "Usage: %s [--shell]", argv[0]);
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
        fprintf(stderr, "Unable to set non-canonical input mode with no echo: %s", strerror(errno));
        exit(1);   
    }

    atexit(restore);

    //Allocate memory to readBuffer
    readBuffer = (char*) malloc(sizeof(char) * BUFFERSIZE);

    
    if (shell_flag)
    {
        //Set up pipes to communicate between the shell and the new terminal
        if (pipe(pipeFromShell) < 0 || pipe(pipeToShell) < 0)
	{
	  fprintf(stderr, "Pipe failed: %s,", strerror(errno));
	  exit(1);
	}

        //Register signal handler for SIGPIPE
        if (signal(SIGPIPE, signal_handler) == SIG_ERR)
	{
	  fprintf(stderr, "Signal registering failed: %s", strerror(errno));
	  exit(1);
	}


        //Fork the program
	processID = fork();

        switch (processID)
        {
            case -1:
                fprintf(stderr, "Unable to fork successfully: %s", strerror(errno));
                exit(1);
                break;
                
            case 0:
            //Child process
            //Duplicate necessary pollArray to redirect stdin, stdout, stderr to and from pipes
            //Close terminal end of pipes
                safeClose(pipeToShell[WRITE_END]);
                safeClose(pipeFromShell[READ_END]);
                safeDup2(pipeToShell[READ_END],   STDIN_FILENO);
                safeDup2(pipeFromShell[WRITE_END], STDOUT_FILENO);
                safeDup2(pipeFromShell[WRITE_END], STDERR_FILENO);
                safeClose(pipeToShell[READ_END]);
                safeClose(pipeFromShell[WRITE_END]);
		
		char *const execOptions[1] = {NULL};

                if (customShell)
                {
		  if (execvp(customShell, execOptions) < 0)
                    {
                        fprintf(stderr, "Unable to start shell: %s", strerror(errno));
                        exit(1);
                    }
                }
                else
                {
                    if (execvp("/bin/bash", execOptions) < 0)
                    {
                        fprintf(stderr, "Unable to start shell: %s", strerror(errno));
                        exit(1);
                    }
                }
                
                if (debug_flag)
                    printf("Child process started, shell started\n");
                break;

            default:
                //Parent Process
                //Close shell end of pipes
                safeClose(pipeToShell[READ_END]);
                safeClose(pipeFromShell[WRITE_END]);
                //Setup polling
                //Keyboard
                pollArray[KEYBOARD].fd = STDIN_FILENO;
                pollArray[KEYBOARD].events = POLLIN;

                //Output from shell
                pollArray[SHELL].fd = pipeFromShell[READ_END];
                pollArray[SHELL].events = POLL_IN | POLL_HUP | POLL_ERR;

                if (debug_flag)
                    printf("Parent process started, polling setup, pipes closed\n");

                readOrPoll();
                break;
        }
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
