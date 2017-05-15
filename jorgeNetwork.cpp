#include <jorgeLua.h>
#include <jorgeNetwork.h>
#include <string.h>
#include <stdlib.h>

// get sockaddr, IPv4 or IPv6:
void *jnet_get_req_ip(struct sockaddr *sa){
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

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
    printf("%s - %s\n", req.verb, req.path);
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