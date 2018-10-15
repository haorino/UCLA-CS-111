/* --- Globals  --- */
// Constants
#ifndef CONSTANTS
#define CONSTANTS

//
#define BUFFERSIZE 256

// Pipe structure
#define WRITE_END 1
#define READ_END 0

// Poll fd array
#define KEYBOARD 0
#define SHELL 1

#define META_SIZE 10
// Meta data for Utility functions
#define FORMAT 0                //Select format shell or stdout
#define PIPE_TO_SHELL_READ 1    // Put in pipe fd
#define PIPE_TO_SHELL_WRITE 2   // Put in pipe fd
#define PIPE_FROM_SHELL_READ 3  // Put in pipe fd
#define PIPE_FROM_SHELL_WRITE 4 // Put in pipe fd
#define PROC_ID 5               // Put in process id
#define SOCKET 6                // Put in socket fd
#define SENDER 7                // Indicate sender of call
#define LOG 8                   // Status of log flag - 1 if selected, 0 if not
#define KEY_FD 9

// Values for meta data
#define DEFAULT -1

#define SHELL_FORMAT 0
#define STDOUT_FORMAT 1

#define CLIENT 0
#define SERVER 1

#define KEY_LENGTH 10

#endif