#ifndef JORGE_NETWORK
#define JORGE_NETWORK

#include <sys/socket.h>
#include <arpa/inet.h>

#define JNET_REQ_BUFF_SIZE 1024

struct jnet_request_header{
    char *field;
    char *value;
    struct jnet_request_header *next;
};

struct jnet_request_data{
  char *verb;
  char *path;
  struct jnet_request_header *header;
  char *body;
};

void *jnet_get_req_ip(struct sockaddr *sa);

//struct jnet_request_data jnet_parse_request(int conn);
char *jnet_parse_request(int conn);

ssize_t jnet_send_all(int conn_fd, char *buffer, size_t *len, int isLast);


#endif
