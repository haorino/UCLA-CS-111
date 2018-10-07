// CS 111 Project 0 Warm Up

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

void signal_handler(int signo)
{
    if (signo == SIGSEGV)
    {
        fprintf(stderr, "Exit code 4: Segmentation fault triggered and handled  - %s", strerror(errno));
        exit(4);
    }
}

int main (int argc, char* argv[])
{
    int argsLeft = 1; //non -1 value to indicate getopt_long is not done
    char* input = NULL;
    char* output = NULL;
    int segfault = 0;

    while (1)
    {
        int option_index = 0;
        static struct option long_options[] = {
            {"input",     required_argument, 0,  'i' },
            {"output",    required_argument, 0,  'o' },
            {"segfault",  no_argument,       0,  's' },
            {"catch",     no_argument,       0,  'c' },
        };

        argsLeft = getopt_long(argc, argv, "", long_options, &option_index);
        
        //Check if any valid arguments are left
        if (argsLeft == -1)
            break;

        //Identify which options were specified
        switch(argsLeft)
        {
            case 'i':
                input = optarg;
                break;
            case 'o':
                output = optarg;
                break;
            case 's':
                segfault = 1;
                break;
            case 'c':
                signal(SIGSEGV, signal_handler);
                break;
            default:
                fprintf(stderr, "Usage: %s [--input=filename] [--output=filename] [--segfault] [--catch] \n",
                    argv[0]);
                exit(1);
        }
    }

    //If input argument is specified redirect to std input
    if (input)
    {
        //Redirect to std input
        int ifd = open(input, O_RDONLY);
        if (ifd >= 0) 
        {
	        close(0);
	        dup(ifd);
	        close(ifd);
        }

        else
        {
            fprintf(stderr, "Exit code 2: Unable to open input file - %s", strerror(errno));
            exit(2);
        }
    }

    //If output argument is specified redirect std output to specified file
    if (output)
    {
         //Redirect to std input
        int ofd = creat(output, S_IRWXU);
        if (ofd >= 0) 
        {
	        close(1);
	        dup(ofd);
	        close(ofd);
        }

        else
        {
            fprintf(stderr, "Exit code 3: Unable to open output file - %s", strerror(errno));
            exit(3);
        }
    }

    //If segfault argument specified cause segmentation fault
    if (segfault)
    {
        char** fault = NULL;
        printf("%s", *fault);        
    }

    //Read from file descripter 0 and write to file descripter 1
    char* buffer;
    buffer = (char*) malloc(sizeof(char));
    ssize_t read_status = read(0, buffer, 1);
    while (read_status > 0)
    {
        write(1, buffer, 1);
        read_status = read(0, buffer, 1);
    }
    free(buffer);
    exit(0);    
}
