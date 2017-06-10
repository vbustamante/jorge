#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#include <jorgeLua.h>
#include <jorgeNetwork.h>
#include <pthread.h>

// TODO embed sqlite just for fun?
// TODO implement openssl
// TODO port over to select on thread pools, instead of spawning worker threads
// TODO make it full http compliant

#define PORT "8080"  // the port users will be connecting to

#define BACKLOG 10   // how many pending connections queue will hold

// A thread running this is spawned for every request 
void *req_thread(void *data){
  // Copy data from main thread
  int conn_fd = *((int *)data);
  free(data);

  // Get request
  char *request; // We need to be able to free the full request data, so we get this
  struct jnet_request_data req_data = jnet_read_request(conn_fd, &request);
  
  printf("do %s on %s through http/1.%c\n", req_data.verb, req_data.path, req_data.version);

  // Defer everything to the Lua subsystem
  jlua_interpret(conn_fd, req_data);

  // Cleanup  
  free(request);
  close(conn_fd);
}

// Entry point
// Sets up the socket and accepts connections.
// the main loop awaits connections e spawns threads for each request.
int main(void){
  
  // Get server info
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
  
  // Create default folders and files
  jlua_setup_environment();
  
  // Get socket
  int sock_fd = -1;
  {
    struct addrinfo *p;
    for(p = servinfo; p != NULL; p = p->ai_next) {
      sock_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    
      if (sock_fd == -1) {
        perror("server: socket");
        continue;
      }

      int reuseAddress=1;
      if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &reuseAddress,
          sizeof(int)) == -1) {
        perror("server: sock options");
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
      fprintf(stderr, "server: failed socket startup\n");
      exit(1);
    }
  }
  freeaddrinfo(servinfo);
  
  //Start socket
  if (listen(sock_fd, BACKLOG) == -1) {
    perror("listen");
    exit(1);
  }

  // Start main loop
  printf("server: waiting for connections...\n");
  bool shutdown = false;  // connected socket file descriptor
  while(!shutdown) {  // main accept() loop
    char incoming_ip[INET6_ADDRSTRLEN]; // since the biggest ip would be ipv6, we allocate that

    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size = sizeof their_addr;

    int *conn_fd = (int *) malloc(sizeof(*conn_fd)); //Put this on the heap so we can pass it to the thread

    // Blocking accept
    // TODO look into select intead of the blocking approach. 
    // The block here makes sense though
    *conn_fd = accept(sock_fd, (struct sockaddr *)&their_addr, &sin_size);
    if (*conn_fd == -1) {
      perror("accept");
      continue;
    }
 
    inet_ntop(
      their_addr.ss_family,
      jnet_get_req_ip((struct sockaddr *)&their_addr),
      incoming_ip, sizeof incoming_ip);

    // Start manager thread    
    printf("\nserver: got connection from %s\n", incoming_ip);

    pthread_t req_thread_pointer;
    pthread_create(&req_thread_pointer, 0, req_thread, conn_fd);
  }

  return 0;
}

