#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#include <jorgeLua.h>

// TODO embed sqlite just for fun?

#define PORT "8080"  // the port users will be connecting to

#define BACKLOG 10   // how many pending connections queue will hold


void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// This is necessary because sommetimes the packets don't get sent entirely.
int sendall(int connection, char *buffer, int *length, int isLast){
  int bytesSent = 0;  
  
  int bytesNow;
  while(bytesSent < *length){
    bytesNow = send(connection, 
      buffer+bytesSent, (*length)-bytesSent , 
      isLast?0:MSG_MORE);
    
    if(bytesNow == -1) break;
    
    bytesSent += bytesNow;
  }
  
  *length = bytesSent;
  
  return bytesNow==-1?-1:0;
}

int main(void){
  
  struct addrinfo *servinfo;
  {
    struct addrinfo hints;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;    // Works for IPv4 and IPv6
    hints.ai_socktype = SOCK_STREAM;// TCP
    hints.ai_flags = AI_PASSIVE;    // Use system's IP

    int status = getaddrinfo(NULL, PORT, &hints, &servinfo);
    if (status != 0) {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
      return 1;
    }
  }
  
  
  int sock_fd;
  {
    int yes=1;
    struct addrinfo *p;
    for(p = servinfo; p != NULL; p = p->ai_next) {
      sock_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    
      if (sock_fd == -1) {
        perror("server: socket");
        continue;
      }

      if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes,
          sizeof(int)) == -1) {
        perror("setsockopt");
        exit(1);
      }

      if (bind(sock_fd, p->ai_addr, p->ai_addrlen) == -1) {
        close(sock_fd);
        perror("server: bind");
        continue;
      }

      break;
    }
    
    if (p == NULL)  {
      fprintf(stderr, "server: failed to bind\n");
      exit(1);
    }
  }
  
  freeaddrinfo(servinfo);
  
  if (listen(sock_fd, BACKLOG) == -1) {
    perror("listen");
    exit(1);
  }
 
  // We override SIGCHLD to avoid the creation of zombie processess by the child 
  struct sigaction sa;
  sa.sa_handler = sigchld_handler; // reap all dead processes
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;

  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    perror("sigaction");
    exit(1);
  }

  printf("server: waiting for connections...\n");

  int conn_fd;  // connected socket file descriptor
  struct sockaddr_storage their_addr; // connector's address information  
  socklen_t sin_size;
  char incoming_ip[INET6_ADDRSTRLEN];
  
  while(1) {  // main accept() loop
    sin_size = sizeof their_addr;
    conn_fd = accept(sock_fd, (struct sockaddr *)&their_addr, &sin_size);
    if (conn_fd == -1) {
      perror("accept");
      continue;
    }

    inet_ntop(
      their_addr.ss_family,
      get_in_addr((struct sockaddr *)&their_addr),
      incoming_ip, sizeof incoming_ip);
    
    printf("server: got connection from %s\n", incoming_ip);

    if (!fork()) { // this is the child process
      close(sock_fd); // child doesn't need the listener
       
      jlua_interpret(conn_fd);
      
      close(conn_fd);
      exit(0);
    }
    close(conn_fd);  // parent doesn't need this
  }

  return 0;
}
