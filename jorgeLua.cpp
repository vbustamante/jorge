#include <jorgeLua.h>
#include <jorgeNetwork.h>

#include <stdlib.h>
#include <string.h>
#include <cmath>

// This global is the way jluaf functions can interact with requests
struct jlua_response_body_node{
  char *data;
  struct jlua_response_body_node *next;
};

struct jlua_global_data{
  int connection;
  jlua_response_body_node *response_body;
  int response_body_length;
  char *response_header;
};

struct jlua_global_data jData;

void jlua_interpret(int conn_fd, char *request){
  
  // Setup connection data
  jData.connection = conn_fd;
  jData.response_body_length = 0;
  jData.response_body = NULL;
  
  // Deal with request data
  
  // Fire up interpreter
  int luaStatus;
  lua_State *L = luaL_newstate();
  
  // TODO Configure libs opening
  luaL_openlibs(L);
  lua_register(L, "echo", jluaf_echo);
  lua_register(L, "setHeader", jluaf_setHeader);

  // Lua push texts

  luaStatus = luaL_loadfile(L, "../main.lua");
  if(luaStatus){
    jlua_print_error(L);
    return;
  }

  luaStatus = lua_pcall(L, 0, 0, 0);
  if(luaStatus){
    // Todo HTTP error on script error
    jlua_print_error(L);
    return;
  }

  luaStatus = luaL_loadfile(L, "../parser.lua");  
  if(luaStatus){
    jlua_print_error(L);
    return;
  }

  luaStatus = lua_pcall(L, 0, 0, 0);
  if(luaStatus){
    // Todo HTTP error on script error
    jlua_print_error(L);
    return;
  }
  
  lua_getglobal(L, "parseRequest");
  lua_pushstring(L, request);
  
  luaStatus = lua_pcall(L, 1, 0, 0);
  if(luaStatus){
    // Todo HTTP error on script error
    jlua_print_error(L);
    return;
  }
  
  
  // Send Header
  size_t headerLen = strlen(jData.response_header);
  jnet_send_all(jData.connection, jData.response_header, &headerLen, 0);

  // Send body
  jlua_response_body_node *walker = jData.response_body;
  jlua_response_body_node *last;

  while(walker != NULL){
    size_t bodyLen = strlen(walker->data);
    jnet_send_all(jData.connection, walker->data, &bodyLen, !walker->next);
    
    last    = walker;
    walker  = walker->next;  

    free(last->data);

    free(last);
  }

  lua_close(L);
}


void jlua_print_error(lua_State* L) {
  // The error message is on top of the stack.
  // Fetch it, print it and then pop it off the stack.
  const char* message = lua_tostring(L, -1);
  fprintf(stderr, "%s\n", message);
  lua_pop(L, 1);
}

int jluaf_echo(lua_State* L){
  
  int args = lua_gettop(L);
  //printf("Echo with %d args\n", args);
  for(int i=1; i<=args; i++){
    
    struct jlua_response_body_node *response_body_end;
    
    response_body_end = 
      (struct jlua_response_body_node*) malloc(sizeof(*response_body_end)+1);
    
    response_body_end->next = NULL;
    response_body_end->data = strdup(lua_tostring(L, i));
    size_t body_bytes = strlen(response_body_end->data);
    jData.response_body_length += body_bytes;

    lua_pushnumber(L, body_bytes);
    // TODO maybe checkout and correct linebreaks
    
    if(jData.response_body){
      struct jlua_response_body_node *walker = jData.response_body;
      
      while(walker->next != NULL) walker = walker->next;
      
      walker->next = response_body_end;
    }else jData.response_body = response_body_end;
  }
  
  return 1;
}

int jluaf_setHeader(lua_State* L){

  if(jData.response_header) free(jData.response_header);
  jData.response_header = strdup(lua_tostring(L, 1));

  lua_pushnumber(L, strlen(jData.response_header));
  return 1;
}