#ifndef JORGE_LUA
#define JORGE_LUA
#include <lua.hpp>

void jlua_interpret(int conn_fd);

void jlua_print_error(lua_State* state);

int jluaf_echo(lua_State* L);

#endif
