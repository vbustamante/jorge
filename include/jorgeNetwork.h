#ifndef JORGE_NETWORK
#define JORGE_NETWORK

#include <sys/socket.h>
#include <arpa/inet.h>

#define JNET_REQ_BUFF_SIZE  1024
#define JNET_RECV_TIMEOUT   .5
#define JNET_RECV_TIMEEXTRA .001

struct jnet_request_header{
  char *field;
  char *value;
  struct jnet_request_header *next;
};

struct jnet_request_data{
  char *verb;
  char *path;
  char version;
  struct jnet_request_header *header;
  char *body;
};


enum jnet_parser_state{
  jnet_parser_state_verb_begin,
  jnet_parser_state_verb_end,
  jnet_parser_state_path_begin,
  jnet_parser_state_path_end,
  jnet_parser_state_version,
  jnet_parser_state_name_begin,
  jnet_parser_state_value,
  jnet_parser_state_halt,
  jnet_parser_state_err
};

void *jnet_get_req_ip(struct sockaddr *sa);

//struct jnet_request_data jnet_parse_request(int conn);
char *jnet_read_request(int conn);

ssize_t jnet_send_all(int conn_fd, char *buffer, size_t *len, int isLast);


#endif
