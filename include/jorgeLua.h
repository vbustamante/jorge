#ifndef JORGE_LUA
#define JORGE_LUA
#include <lua.hpp>

#define JLUA_SCRIPT_PATH "../"

void jlua_interpret(int conn_fd, char* request);

void jlua_print_error(lua_State* state);

int jluaf_echo(lua_State* L);

int jluaf_setHeader(lua_State* L);

#endif
