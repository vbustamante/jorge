#include <jorgeLua.h>
#include <jorgeNetwork.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>

// get sockaddr, IPv4 or IPv6:
void *jnet_get_req_ip(struct sockaddr *sa){
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


struct buffer_node{
    char data[JNET_REQ_BUFF_SIZE];
    struct buffer_node* next;
};

char *jnet_read_request(int conn){

  ssize_t totalBytes = 0;
  struct buffer_node *first_buffer = (struct buffer_node*) malloc(sizeof *first_buffer);
  struct buffer_node *last_buffer = first_buffer;
  last_buffer->next = NULL;
  last_buffer->data[0] = '\0';

  int loops = 0;
  struct timeval begin;
  gettimeofday(&begin , NULL);

  while (true) {
    loops++;
    struct timeval now;
    gettimeofday(&now , NULL);
    double elapsedSeconds = (now.tv_sec - begin.tv_sec) + 1e-6 * (now.tv_usec - begin.tv_usec);

    //if you got some data, then wait a little more time
    if( totalBytes > 0 && elapsedSeconds > JNET_RECV_TIMEEXTRA ) break;
    //if you got no data at all, wait timeout
    else if( elapsedSeconds > JNET_RECV_TIMEOUT) break;

    if(!last_buffer->next){
      last_buffer->next = (struct buffer_node*) malloc(sizeof *last_buffer->next);
      last_buffer->next->next = NULL;
      last_buffer->next->data[0] = '\0';
    }

    ssize_t recvBytes = recv(conn, last_buffer->data, JNET_REQ_BUFF_SIZE-1, MSG_DONTWAIT);
    // NONPORTABLE MSG_DONTWAIT makes single call non blocking, but is only supported on linux

    if(recvBytes > 0) {
      totalBytes += recvBytes;
      //printf("%d: \n%s\n(%zu out of %d)\n", loops, last_buffer->data, recvBytes, JNET_REQ_BUFF_SIZE-1);
      last_buffer = last_buffer->next;
      gettimeofday(&begin, NULL);
    }
    else usleep(100000); // Wait a little if nothing was received
  }
  //printf("bytes: %zu\n", totalBytes);
  char *request = (char *)malloc((totalBytes+1) * (sizeof *request));
  char *endofstring = request;
  struct buffer_node *walker;
  walker = first_buffer;
  while(walker->data[0] != '\0'){
    strcpy(endofstring, walker->data);
    endofstring += strlen(walker->data);

//    printf("----------------------------------\n");
//    printf("\nSOURCE := %s\n", walker->data);
//    printf("\nTHIS := %s\n", endofstring);
//    printf("\nFULL := %s\n", request);

    last_buffer = walker;
    walker = walker->next;
    free(last_buffer);
  }
  free(walker);

  return request;
}
/*
struct jnet_request_data jnet_parse_request(int conn){

  ssize_t readBytes = JNET_REQ_BUFF_SIZE -1;
  char buffer[JNET_REQ_BUFF_SIZE];
  size_t buffer_offset = 0;
  bool first_chunk = true;

  struct jnet_request_data req;
  req.verb = NULL;

  while(readBytes == JNET_REQ_BUFF_SIZE-1){
    readBytes = recv(conn, buffer + buffer_offset, JNET_REQ_BUFF_SIZE-1-buffer_offset, 0);
    if(readBytes != JNET_REQ_BUFF_SIZE-1){ //TODO end reads another way
      buffer[readBytes]='\0';
    }
    //printf("%s", buffer);

    // Parse Header
    // TODO error checking on request parsing
    size_t segment=0, offset=0;

    if(first_chunk){ // Only Parse first line if this is the first chunk of the request
      // VERB
      while(buffer[offset] <= 'Z' && buffer[offset] >= 'A') offset++;
      if(buffer[offset] != ' ' && buffer[offset] != '\t') break;
      buffer[offset++] = '\0';
      req.verb = strdup(buffer);
      while(buffer[offset] == ' ' || buffer[offset] == '\t') offset++;
      segment+=offset;

      // PATH
      offset=0;
      while(buffer[segment+offset] != ' ' && buffer[segment+offset] != '\t') offset++;
      buffer[segment+offset] = '\0';
      req.path = strdup(buffer + segment);
      while(buffer[segment+offset] != '\n') offset++; //ignore http version and go to next line
      segment+=offset + 1;
    }

    // Headers
    bool chunk_ended = false; // Chunk ended before header
    bool header_ended = false;// Header was returned completely
    while(!chunk_ended || !header_ended){
      size_t value_offset =0;
      bool line_ended = false;
      bool await_value = false;

      for(offset = 0; !line_ended; offset++) {
        //printf("%zu : %c\n", segment+offset, buffer[segment+offset]);
        switch(buffer[segment+offset]){
          case '\0': // Chunk ended during header
            chunk_ended= true;
            line_ended = true;
            first_chunk = false;
            //strcpy();
            break;
          case ':':
            buffer[segment+offset] = '\0';
            await_value = true;
            break;
          case '\n':
            if((offset) >= 1 && buffer[segment+offset-1] == '\r') {
              line_ended = true;
              if(offset == 1) header_ended=true;
              else buffer[segment+offset-1] = '\0';
            }
            break;
          default:
            if(await_value && buffer[segment+offset] != ' ' && buffer[segment+offset] != '\t'){
              await_value = false;
              value_offset = offset;
            }
        }
      }

      if(!chunk_ended){
        //printf("header|%s|%s|\n", buffer + segment, buffer + segment + value_offset);
        struct jnet_request_header *request_header = (struct jnet_request_header*) malloc(sizeof(*request_header));
        request_header->next = NULL;
        request_header->field = strdup(buffer+segment);
        request_header->value = strdup(buffer+segment + value_offset);

        if(req.header){
          struct jnet_request_header *walker = req.header;
          while(walker->next != NULL) walker = walker->next;
          walker->next = request_header;
        }else req.header = request_header;
      }
      segment+=offset;
    }

    // TODO fix the tail \n header element the parser generates
    printf("|%s|- |%s|\n", req.verb, req.path);
    struct jnet_request_header *header_walker = req.header;
    while(header_walker != NULL){
      printf("%s(%zu): %s(%zu)\n",
             header_walker->field, strlen(header_walker->field),
             header_walker->value, strlen(header_walker->value));
      header_walker = header_walker->next;
    }



    // TODO parse body
  }

  return req;
}
*/

// This is necessary because sometimes the packets don't get sent entirely.
ssize_t jnet_send_all(int connection, char *buffer, size_t *length, int isLast){
  size_t bytesSent = 0;
  ssize_t bytesNow=0;

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
