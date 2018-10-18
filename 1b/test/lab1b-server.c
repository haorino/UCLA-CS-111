
/*
Name: Rohan Varma
ID: 111111111
email: rvarm1@ucla.edu
*/
#include <termios.h>
#include <mcrypt.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/poll.h>
#include <signal.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <mcrypt.h>
#include <sys/stat.h>

int encrypt_flag = 0;
char *e_key;
char *encrypt_key_file;
MCRYPT td;
int KEY_LEN = 16;
int kfd = -1;
char *IV;
int debug = 0;
int TO_SHELL = 1;
int TO_SOCKET = 0;
int to_shell[2]; //parent -> child communication. reads from term and write to shell
int from_shell[2]; //shell out -> writ to terminal
pid_t cpid = -1;

/** Utiltiy functions **/
void err_exit(const char *msg, int num) {
  fprintf(stderr, "%s: error = %d, message = %s", msg, errno, strerror(errno));
  exit(EXIT_FAILURE);
}

void print_ops_and_exit() {
  fprintf(stderr, "Usage: server --port=portnum [--encrypt=keyfile] \n");
  exit(EXIT_FAILURE);
}

/** Safe reading/writing functiions **/

int safe_read(int fd, char *buf, int s) {
  ssize_t nbytes = read(fd, buf, s);
  if (nbytes < 0) err_exit("read error", errno);
  return nbytes;
}

int safe_write(int fd, char *buf, int s) {
  ssize_t nbytes = write(fd, buf, s);
  if (nbytes < 0) err_exit("write error", errno);
  return nbytes;
}

/** Child process clean up function. **/
void shell_wait_and_clean() {
  int stat_loc;
  pid_t waited = waitpid(cpid, &stat_loc, 0);
  if (waited == -1) err_exit("waitpid failed", errno);
  if (debug) printf("done waiting \n");
  fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", WTERMSIG(stat_loc), WEXITSTATUS(stat_loc));
  exit(EXIT_SUCCESS);
}
/** Signal handler **/
void handler(int sig) {
  if (debug) printf("in signal handler!!!");
  if (sig == SIGTERM && debug) {
    printf("in handler with SIGTERM \n");
  }
  if (sig == SIGINT && debug) {
    printf("in handler with SIGINT \n");
  }
  if(sig == SIGPIPE && debug) {
    shell_wait_and_clean();
    printf("in handler with SIGPIPE \n");
  }
  //shell_wait_and_clean();
  exit(EXIT_SUCCESS);
}
/** encryption initializer **/
void setup_encryption() {
  td = mcrypt_module_open("twofish", NULL, "cfb", NULL); //open lib
  if(td == MCRYPT_FAILED) err_exit("mcrypt lib error", errno);
  //assign memory and read key
  kfd= open(encrypt_key_file, O_RDONLY); 
  if(kfd < 0) err_exit("keyfile open error", errno);
  e_key = malloc(KEY_LEN); 
  if(e_key == NULL) err_exit("malloc for key failed", errno);
  int iv_size = mcrypt_enc_get_iv_size(td);
  IV = malloc(iv_size);
  if(IV==NULL) err_exit("malloc for IV failed", errno);
  memset(e_key, 0, KEY_LEN); //zero out before reading hte key
  int bytes_read = read(kfd, e_key, KEY_LEN);
  if(bytes_read != KEY_LEN) err_exit("couldn't read key", errno);
  int x = close(kfd); 
  if (x < 0) err_exit("close failed", errno);
  int i;
  for(i=0;i<iv_size;i++) IV[i]=0; //should definitely not be done like this in production cases
  if(mcrypt_generic_init(td,e_key, KEY_LEN, IV) < 0) err_exit("couldn't init encrypt", errno);
  free(e_key);
  free(IV);
}

void do_write(char *buf, int ofd, int nbytes, int to) {
  int i;
  for (i = 0; i < nbytes; i++) {
    char ch = *(buf + i);
    if (ch == 0x03) {
      if (to == TO_SHELL) {
        if (debug) printf("about to send SIGINT to shell \n");
        if (kill(cpid, SIGINT) < 0) err_exit("KILL FAILED", errno);
      }
    }
    if (ch == 0x04) {
      if(to == TO_SHELL) {
        if (debug) printf("about to close writing to shell");
        close(to_shell[1]);
      }
    }
    else {
      safe_write(ofd, buf+i, 1);
    }
  }
}
int main(int argc, char **argv) {
  int opt;
  int port_flag = 0;
  int port_num;
  int sockfd, newsockfd, n;
  static struct option long_options[] = {
    {"port", required_argument, 0, 'p'},
    {"encrypt", required_argument, 0, 'e'},
    {"debug", no_argument, 0, 'd'},
    {0, 0, 0, 0}
  };
  while((opt = getopt_long(argc, argv, "p:e:d", long_options, NULL)) != -1) {
    switch(opt) {
    case 'p':
      port_flag = 1;
      port_num = atoi(optarg);
      break;
    case 'e':
      encrypt_flag = 1;
      encrypt_key_file = optarg;
      break;
    case 'd':
      debug = 1;
      break;
    default:
      print_ops_and_exit();
    }
  }
  if (debug) {
    printf("args specified \n");
    if (port_flag) printf("port: %d\n", port_num);
    if (encrypt_flag) printf("encrypt: %s \n", encrypt_key_file);
  }
  if (!port_flag) print_ops_and_exit();
  if(encrypt_flag) {
    setup_encryption();
  }
  if (signal(SIGINT, &handler) == SIG_ERR) err_exit("signal failed", errno);
  if(signal(SIGPIPE, &handler) == SIG_ERR) err_exit("signal failed", errno);
  if(signal(SIGTERM, &handler) == SIG_ERR) err_exit("signal failed", errno);
  if (debug) printf("signal handlers set up \n");
  struct sockaddr_in serv_addr, cli_addr; //structures containing the addresses
  sockfd = socket(AF_INET, SOCK_STREAM, 0); //create a STREAM socket (chars read in as cont. stream)
  if (sockfd < 0) err_exit("error opening socket", errno);
  if (debug) printf("created socket, sockfd is: %d \n", sockfd);
  memset((char *) &serv_addr, 0, sizeof(serv_addr)); //zero out the server address
  serv_addr.sin_family = AF_INET; //address family
  serv_addr.sin_port = htons(port_num); //port number converted to network byte order
  serv_addr.sin_addr.s_addr = INADDR_ANY; //IP addr of host, = addr of machine its running on
  //bind socket to address of current host
  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    err_exit("binding error \n", errno);
  }
  if(debug) printf("bind happened successfully \n");
  //listen for connections. max backlog is 5
  if(listen(sockfd, 5) < 0) err_exit("invalid socket error \n", errno);
  if (debug) printf("listened exited successfully \n");
  unsigned int clilen = sizeof(cli_addr);
  if (debug) printf("accept exited successfully \n");
  //block unitl client connects. return fildes when connection is established
  newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
  if (newsockfd < 0) err_exit("accept error\n", errno);
  if (debug) printf("accepted a connection \n");
  if(pipe(to_shell) < 0) err_exit("pipe error", errno);
  if(pipe(from_shell) < 0) err_exit("pipe err", errno);
  cpid = fork();
  if(cpid < 0) err_exit("fork() failed!", errno);
  if(cpid == 0) {
    //won't be needing these pipes
    if(close(to_shell[1]) < 0) err_exit("close failed", errno);
    if(close(from_shell[0]) < 0) err_exit("close failed", errno);
    //make stdin the read end of our pipe
    //read from the terminal -> make stdin the read end of our parent to child pipe
    if(dup2(to_shell[0], STDIN_FILENO) < 0) err_exit("dup2 failed", errno);
    if(close(to_shell[0]) < 0) err_exit("close failed", errno);
    //write to terminal -> make stdout and erro the write end of our child to parent pipe
    if(dup2(from_shell[1], STDOUT_FILENO) < 0) err_exit("dup2 failed", errno);
    if(dup2(from_shell[1], STDERR_FILENO) < 0) err_exit("dup2 failed", errno); //now err and out refer to the fildes from_shell[1]
    if(close(from_shell[1]) < 0) err_exit("close failed", errno); //close because we duped it
    int e = execvp("/bin/bash", NULL);
    if (e < 0) err_exit("exec failed \n", errno);
  }
  else {
    //parent process
    if(close(to_shell[0])<0) err_exit("close failed", errno);
    if(close(from_shell[1])<0) err_exit("close failed", errno);
    //input received through the network socket should be forwarded through the pipe to the shell.
    //input received from the shell pipes (which receive both stdout and stderr from the shell) should be forwarded out to the network socket.
    struct pollfd foo[2];
    foo[0].fd = newsockfd; //read from this and write to to_shell[1]
    foo[0].events = POLLIN | POLLHUP | POLLERR;
    foo[1].fd = from_shell[0];
    foo[1].events = POLLIN | POLLHUP | POLLERR;
    while(1) {
      int r = poll(foo, 2, 0);
      if(r < 0) err_exit("poll failed", errno);
      if(r ==0) continue;
      if(foo[0].revents & POLLIN) {
        //we have some stuff on newsockfd that we need to write to shell
        if(debug) printf("got stuff from newsockfd, going to read it and write it to shell \n");
        //read from newsock fd
        char from_sock[256];
        int bread = read(newsockfd, from_sock, 256);
        if(bread <= 0) {
          //means we got EOF or read error
          if(debug)  printf("server got EOF or read error from client \n");
          if(kill(cpid, SIGTERM) < 0) err_exit("KILL FAILED", errno);
          shell_wait_and_clean();
          break;
        }
        if(encrypt_flag) {
          //decrypt before writing to shell if we must
          int j;
          for(j=0;j<bread;j++){
	    if(from_sock[j]!= '\r' && from_sock[j]!= '\n') {
	      if(mdecrypt_generic(td, &from_sock[j], 1) != 0) err_exit("encrypt", errno);
            }
          }
        }
        do_write(from_sock, to_shell[1], bread, TO_SHELL);
      }
      if (foo[1].revents & POLLIN) {
	//we have stuff from the shell pipe which we need to write to socket
	if(debug) printf("got stuff from shell, going to read it and write it to socket\n");
	char from_shell_buf[256];
	int bread = safe_read(from_shell[0], from_shell_buf, 256);
	if(bread ==0) {
	  //this means that the server got EOF from shell.
	  if (debug) printf("shell sent EOF");
	  shell_wait_and_clean(); break;
	}
	if(encrypt_flag) {
	  //encrypt if we must
	  //if(debug) printf("GOING TO ENCRYPT \n");
	  if(mcrypt_generic(td, &from_shell_buf, bread) != 0) err_exit("encrypt", errno);
	}
	//write to the socket
	do_write(from_shell_buf, newsockfd, bread, TO_SOCKET);
      }
      if(foo[1].revents & (POLLHUP | POLLERR)) {
	//error handling
	if (debug) printf("received err from shell \n");
	if(close(foo[1].fd) < 0 ) err_exit("close failed", errno);
	shell_wait_and_clean();
	break;
      }
    }
  }
}
