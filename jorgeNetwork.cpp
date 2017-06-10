#include <jorgeLua.h>
#include <jorgeNetwork.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>

// get sockaddr, IPv4 or IPv6:
void *jnet_get_req_ip(struct sockaddr *sa){
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// Linked list of buffers to store recv data
struct buffer_node{
    char data[JNET_REQ_BUFF_SIZE];
    struct buffer_node* next;
};

// Receives a connected socket and receives all data until timeout
// or until no data is coming.
struct jnet_request_data jnet_read_request(int conn, char *request){

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
  request = (char *)malloc((totalBytes+1) * (sizeof *request));
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

  // TODO Http parser
  /*  At this point the whole request should be in `char *request`
   *  In jorgeNetwork.h there is both the request struct and the enum
   *  for the parser state machine.
   *  The main idea here is to take advantage of the contiguous request string
   *  and only set the pointers on the struct to those memory locations.
   *  TODO Also refactor this function so it returns the struct instead of the string
   */
  struct jnet_request_data req_data;
  int httpParserState =  jnet_parser_state_verb_begin;
  int parserIndex = 0;
  
  while(
    (httpParserState != jnet_parser_state_halt) &&
    (httpParserState != jnet_parser_state_err)
  ){
    char *thisChar = &(request[parserIndex]);
    char caractere = *thisChar; 
    switch (httpParserState){
      case jnet_parser_state_verb_begin:
        if(*thisChar == ' ' || *thisChar == '\t'){}
        else if(
          (*thisChar >= 'A' && *thisChar <= 'Z') ||
          (*thisChar >= 'a' && *thisChar <= 'z')
        ){
          req_data.verb = thisChar;
          httpParserState = jnet_parser_state_verb_end;
        }else{
          httpParserState = jnet_parser_state_err;          
        }
        parserIndex++;
      break;
      
      case jnet_parser_state_verb_end:
        if(*thisChar == ' ' || *thisChar == '\t'){     
          *thisChar = '\0';
          httpParserState = jnet_parser_state_path_begin;          
        }
        else if(
          (*thisChar >= 'A' && *thisChar <= 'Z') ||
          (*thisChar >= 'a' && *thisChar <= 'z') 
        ){}
        else{
          httpParserState = jnet_parser_state_err;
        }
        
        parserIndex++;
      break;
      
      case jnet_parser_state_path_begin:
        if(*thisChar == ' ' || *thisChar == '\t'){}
        else if(
          (*thisChar == '/')
        ){
          req_data.path = thisChar;
          httpParserState = jnet_parser_state_path_end;  
        }
        else{
          httpParserState = jnet_parser_state_err;
        }
        
        parserIndex++;
      break;
      
      case jnet_parser_state_path_end:
        if(*thisChar == ' ' || *thisChar == '\t'){     
          *thisChar = '\0';
          httpParserState = jnet_parser_state_version;          
        }
        else if(
          (*thisChar >= 'A' && *thisChar <= 'Z') ||
          (*thisChar >= 'a' && *thisChar <= 'z') ||
          (*thisChar >= '0' && *thisChar <= '9') ||
          (*thisChar == '/')||(*thisChar == '.') ||
          (*thisChar == '+')||(*thisChar == '-') ||
          (*thisChar == '=')||(*thisChar == '?') ||
          (*thisChar == '~')||(*thisChar == '%')
        ){}
        else{
          httpParserState = jnet_parser_state_err;
        }
        parserIndex++;
      break;
      case  jnet_parser_state_version:
        if(*thisChar == ' ' || *thisChar == '\t') parserIndex++;
        else if(
          toupper(*(thisChar))   == 'H' &&
          toupper(*(thisChar+1)) == 'T' &&
          toupper(*(thisChar+2)) == 'T' &&
          toupper(*(thisChar+3)) == 'P' &&
          *(thisChar+4)          == '/' &&
          *(thisChar+5)          == '1' &&
          *(thisChar+6)          == '.' &&
          ((*(thisChar+7)=='0') || (*(thisChar+7)=='1')) && // 0 or 1
          *(thisChar+8)          == '\r' &&  
          *(thisChar+9)          == '\n'  
        ){
          req_data.version = *(thisChar+7);
          httpParserState = jnet_parser_state_halt; // Not parsing headers yet
          parserIndex += 10;
        }
        else{
          httpParserState = jnet_parser_state_err;
          parserIndex++;
        }
      break;
        
      default:
        httpParserState = jnet_parser_state_halt;
    }
  }

  return req_data;
}

// Receives string of data and a connected socket and sends it all.
// This is necessary because sometimes the packets don't get sent entirely.
ssize_t jnet_send_all(int connection, char *buffer, size_t *length, int isLast){
  size_t bytesSent = 0;
  ssize_t bytesNow = 0;

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
