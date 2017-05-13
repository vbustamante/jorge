#include <jorgeLua.h>

#include <sys/socket.h>
#include <string.h>

int glob_conn_fd; // This global is the way jluaf functions can do network stuff
void jlua_interpret(int conn_fd){
  
  glob_conn_fd = conn_fd;
  
  int luaStatus;
  lua_State *L = luaL_newstate();
  
  luaL_openlibs(L);
  lua_register(L, "echo", jluaf_echo);
  
  luaStatus = luaL_loadfile(L, "../main.lua");
  if(luaStatus){
    jlua_print_error(L);
    return;
  }

  luaStatus = lua_pcall(L, 0, LUA_MULTRET, 0);
  if(luaStatus){
    jlua_print_error(L);
    return;
  }
  
  lua_close(L);
}


void jlua_print_error(lua_State* L) {
  // The error message is on top of the stack.
  // Fetch it, print it and then pop it off the stack.
  const char* message = lua_tostring(L, -1);
  fprintf(stderr, message);
  lua_pop(L, 1);
}

int jluaf_tst(lua_State* L){
  int args = lua_gettop(L);
  
  printf("tst() was called with %d arguments:\n", args);
  
  for(int n=1; n<=args; n++){
    printf(" Argument %d: '%s'\n", n, lua_tostring(L, n));
  }
  
  lua_pushnumber(L, 123);
  
  return 1;
}

int jluaf_echo(lua_State* L){
  
  int args = lua_gettop(L);
  int msgLen, retValue = 0;
 
  for(int i=1; i<=args; i++){

    msgLen = strlen(lua_tostring(L, i));
    
    int tValue = send(glob_conn_fd, lua_tostring(L, i), msgLen, i==args?0:MSG_MORE);
    
    if(tValue == -1){
      retValue = tValue;
      break;
    }else retValue += tValue;
  }
  
  lua_pushnumber(L, retValue);
  
  return 1;
}
