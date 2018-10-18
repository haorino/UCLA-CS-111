/*
Name: Rohan Varma
ID: 111111111
email: rvarm1@ucla.edu
*/
#include <termios.h>
#include <stdlib.h>
#include <mcrypt.h>
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
#include <sys/stat.h>

int encrypt_flag = 0;
char *e_key;
char *encrypt_key_file;
MCRYPT td;
int KEY_LEN = 16;
int kfd;
char *IV;
char buf[256];
int debug = 0;
int TO_SERVER = 10;
int FROM_SERVER = 5;
int log_file_fd;
pid_t cpid;
struct termios saved_state;

void print_ops_and_exit() {
  fprintf(stderr, "Usage: client --port=portnum [--log=logfile] [--encrypt=keyfile]\n");
  exit(EXIT_FAILURE);
}

void err_exit(const char *msg, int num) {
  fprintf(stderr, "%s: error = %d, message = %s", msg, errno, strerror(errno));
  exit(EXIT_FAILURE);
}

//** Non-canonical terminal saving/restoring functions **//
void save_normal_settings() {
  if(debug) printf("saving normal terminal settings \n");
  int s = tcgetattr(STDIN_FILENO, &saved_state);
  if (s < 0) {
    err_exit("Error getting terminal params", errno);
  }
}

void restore_normal_settings() {
  if (debug) printf("restoring normal terminal settings upon exit \n");
  int s = tcsetattr(STDIN_FILENO, TCSANOW,  &saved_state);
  if (s < 0) {
    err_exit("error restoring original terminal params", errno);
  }
}

/** reads and writes that check for sys errors **/

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

void do_write(char *buf, int ofd, int nbytes) {
  int i;
  for(i = 0; i < nbytes; i++) {
    char ch = *(buf+i);
    if(ch == '\r' || ch == '\n') {
      if(ofd == STDOUT_FILENO) {
        char tmp[2]; tmp[0] = '\r'; tmp[1] = '\n';
        safe_write(ofd, tmp, 2);
      }
      else {
        char tmp[0]; tmp[0] = '\n';
        safe_write(ofd, tmp, 1);
      }
    }
    else {
      safe_write(ofd, buf + i, 1);
    }
  }
}
void do_write_log(char *buf, int nbytes, int where) {
  if (where == TO_SERVER) {
    if(dprintf(log_file_fd, "SENT %d bytes: ", nbytes)<0) err_exit("write to log failed", errno);
    do_write(buf, log_file_fd, nbytes);
    safe_write(log_file_fd, "\n", 1);
  }
  if (where == FROM_SERVER) {
    if(dprintf(log_file_fd, "RECEIVED %d bytes: ", nbytes)<0) err_exit("write to log failed", errno);
    do_write(buf, log_file_fd, nbytes);
    safe_write(log_file_fd, "\n", 1);
  }
}

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


int main(int argc, char **argv) {
  int opt;
  char *log_file;
  int log_flag = 0;
  int port_num;
  int port_flag = 0;
  static struct option long_options[] = {
    {"port", required_argument, 0, 'p'},
    {"log", optional_argument, 0, 'l'},
    {"encrypt", required_argument, 0, 'e'},
    {"debug", no_argument, 0, 'd'},
    {0, 0, 0, 0}
  };
  while((opt = getopt_long(argc, argv, "p:l:e:d", long_options, NULL)) != -1) {
    switch(opt){
    case 'p':
      port_num = atoi(optarg);
      port_flag = 1;
      break;
    case 'l':
      log_file = optarg;
      log_flag = 1;
      break;
    case 'e':
      encrypt_key_file = optarg;
      encrypt_flag = 1;
      break;
    case 'd':
      debug = 1;
      break;
    default:
      print_ops_and_exit();
    }
  }
  if (debug) {
    printf("args specified: \n");
    if(port_flag) {
      printf("port: %d\n", port_num);
    }
    if(log_flag) {
      printf("log: %s \n", log_file);
    }
    if(encrypt_flag) {
      printf("encrypt: %s \n", e_key);
    }
  }
  if(!port_flag) {
    print_ops_and_exit();
  }
  if(log_flag) {
    log_file_fd = creat(log_file, S_IRWXU);
    if(log_file_fd < 0) err_exit("creat failed", errno);
  }
  if(encrypt_flag) {
    setup_encryption();
  }
  save_normal_settings();
  atexit(restore_normal_settings);
  struct termios non_canonical_input_mode;
  // put current state of terminal into new termios struct, then manually change it
  tcgetattr(STDIN_FILENO, &non_canonical_input_mode);
  non_canonical_input_mode.c_iflag = ISTRIP; //only lower 7 bits
  non_canonical_input_mode.c_oflag = 0; // no processing
  non_canonical_input_mode.c_lflag = 0; // no processing
  int s = tcsetattr(STDIN_FILENO, TCSANOW, &non_canonical_input_mode);
  if (s < 0) {
    err_exit("error setting terminal \n", errno);
  }
  if (debug) {
    printf("putting into non non_canonical_input_mode \n");
  }
  struct sockaddr_in serv_addr; //contains address of server
  struct hostent *server = gethostbyname("localhost");
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) err_exit("error opening socket", errno);
  if (debug) printf("created socket, sockfd is: %d \n", sockfd);
  memset((char *) &serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port_num);
  memcpy((char *) &serv_addr.sin_addr.s_addr, (char *) server->h_addr, server->h_length);
  /* Now connect to the server */
  if(connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) err_exit("error connecting \n", errno);
  if(debug) printf("connect() was called \n");
  struct pollfd foo[2];
  foo[0].fd = STDIN_FILENO;
  foo[0].events = POLLIN | POLLHUP | POLLERR;
  foo[1].fd = sockfd;
  foo[1].events = POLLIN | POLLHUP | POLLERR;
  while (1) {
    int r = poll(foo, 2, 0);
    if (r < 0) err_exit("poll failed", errno);
    if (r == 0) continue;
    if (foo[0].revents & POLLIN) {
      if (debug) printf("got stuff from standard in \n");
      char from_stdin[256];
      int bytes_read = safe_read(STDIN_FILENO, from_stdin, 256);
      do_write(from_stdin, STDOUT_FILENO, bytes_read);
      if(encrypt_flag) {
        //one by one encryption before we write to socket
        int k;
        for(k=0;k<bytes_read;k++) {
          if(from_stdin[k]!='\r' && from_stdin[k] != '\n') {
            if(mcrypt_generic(td, &from_stdin[k], 1) != 0) err_exit("encryption", errno);
          }
        }
      }
      if(log_flag) do_write_log(from_stdin, bytes_read, TO_SERVER);
      do_write(from_stdin, foo[1].fd, bytes_read); //this is written to the server
    }
    if (foo[1].revents & POLLIN) {
      //received something from the server
      char from_server[256];
      int bytes_read = safe_read(foo[1].fd, from_server, 256);
      if (bytes_read == 0) {
        if(debug) printf("we prob have a ^C or ^D \n");
        if(close(sockfd) < 0) err_exit("close failed", errno);
        exit(EXIT_SUCCESS);
      }
      if(log_flag) do_write_log(from_server, bytes_read, FROM_SERVER);
      if(debug) printf("got stuff from server \n");
      if(encrypt_flag) { //need to decrypt
	//printf("GOING TO DECRYPT");
	if(mdecrypt_generic(td, &from_server, bytes_read) != 0) err_exit("encrypt", errno);
      }
      do_write(from_server, STDOUT_FILENO, bytes_read);
    }
    if (foo[1].revents & (POLLHUP | POLLERR)) {
      if (debug) printf("got error from server \n");
      int x = close(sockfd);
      if(x < 0) err_exit("close failed", errno);
      exit(EXIT_SUCCESS);
    }
  }
}
