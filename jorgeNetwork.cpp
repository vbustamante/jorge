#include <jorgeLua.h>
#include <jorgeNetwork.h>
#include <string.h>

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
    printf("%s", buffer);

    // Parse Header
    // TODO error checking on request parsing
    size_t segment=0, offset=0;

    if(first_chunk){ // Only Parse first line if this is the first chunk of the request
      // VERB
      while(buffer[offset] <= 'Z' && buffer[offset] >= 'A') offset++;
      if(buffer[offset] != ' ' && buffer[offset] != '\t') break;
      buffer[offset] = '\0';
      req.verb = strdup(buffer);
      while(buffer[offset] == ' ' || buffer[offset] == '\t') offset++;
      segment+=offset;

      // PATH
      offset=0;
      while(buffer[segment+offset] != ' ' && buffer[offset] != '\t') offset++;
      buffer[segment+offset] = '\0';
      req.path = strdup(buffer + segment);
      while(buffer[offset] != '\n') offset++; //ignore http version and go to next line
      segment+=offset + 1;
    }

    // Headers
    bool chunk_ended = false;
    while(!chunk_ended){

      size_t value_offset;
      bool got_cr = false;
      bool line_ended = false;
      bool await_value = false;
      for(offset = 0; !line_ended; offset++) {
        switch(buffer[segment+offset]){
          case '\0': // Chunk ended during header
            chunk_ended=true;
          case ':':
            buffer[segment+offset] = '\0';
            await_value = true;
            break;
          case '\r':
            got_cr = true;
          case '\n':
            if(got_cr) {
              line_ended = true;
              if(offset == 1) chunk_ended=true;
            }

          default:
            if(await_value && buffer[segment+offset] != ' ' && buffer[segment+offset] == '\t')
              value_offset = offset;
            break;
        }

        segment+=offset; // INFINITE LOOP SOMEWHERE

      }
    }



    // Parse body?
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