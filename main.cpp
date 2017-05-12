#include "stdio.h"
#include <lua.hpp>


// TODO Look into boost asio and boost thread to make the server
// TODO embed sqlite just for fun?

static void close_state(lua_State **L) { lua_close(*L); }
#define cleanup(x) __attribute__((cleanup(x)))
#define auto_lclose cleanup(close_state) 

void print_error(lua_State* state) {
  // The error message is on top of the stack.
  // Fetch it, print it and then pop it off the stack.
  const char* message = lua_tostring(state, -1);
  puts(message);
  lua_pop(state, 1);
}

int echo(lua_State* L){
  int args = lua_gettop(L);
  
  printf("howdy() was called with %d arguments:\n", args);
  
  for(int n=1; n<=args; n++){
    printf(" Argument %d: '%s'\n", n, lua_tostring(L, n));
  }
  
  lua_pushnumber(L, 123);
  
  return 1;
}

int main(void){
  int luaStatus;
  
  lua_State *L = luaL_newstate();
  
  luaL_openlibs(L);
  lua_register(L, "echo", echo);

  
  luaStatus = luaL_loadfile(L, "../main.lua");
  if(luaStatus){
    print_error(L);
    return 1;
  }

  luaStatus = lua_pcall(L, 0, LUA_MULTRET, 0);
  if(luaStatus){
    print_error(L);
    return 1;
  }
  
  return 0;
}
