#ifndef JORGE_LUA
#define JORGE_LUA "jorgeLua.h"
#include <lua.hpp>
#include <jorgeNetwork.h>
#include <sys/stat.h>

#define JLUA_SCRIPT_PATH "jorgeScripts"

void jlua_setup_environment();

void jlua_interpret(int conn_fd, struct jnet_request_data request);

void jlua_print_error(lua_State* state);

int jluaf_echo(lua_State* L);

int jluaf_setHeader(lua_State* L);

int jluaf_sqlQuery(lua_State *L);

#define JLUA_HELLOW_TEXT  "local header = [[\n\
HTTP/1.1 200 OK\n\
Server: Jorge/0.2\n\
Content-Length: $bodyLen\n\
Content-Type: text/html\n\n\
]]\n\
\n\
local body = [[\n\
<!DOCTYPE html>\n\
<html>\n\
  <head>\n\
    <title>Jorge Root</title>\n\
  </head>\n\
  <body>\n\
    <h1>TONY WORLD!</h1>\n\
  </body>\n\
</html>\n\
]]\n\
\n\
local bodyLen = echo(body)\n\
\n\
local header = header:gsub('\\n', '\\r\\n'):gsub('$bodyLen', bodyLen)\n\
local headerLen = setHeader(header)\n\
\n\
io.write('Lua - '..bodyLen+headerLen..' bytes sent, H:'..headerLen..' & B:'..bodyLen..'\\n')"

#define JLUA_404_TEXT  "local header = [[\n\
HTTP/1.1 404 NOT FOUND\n\
Server: Jorge/0.2\n\
Content-Length: $bodyLen\n\
Content-Type: text/html\n\n\
]]\n\
\n\
local body = [[\n\
<!DOCTYPE html>\n\
<html>\n\
  <head>\n\
    <title>Not Found</title>\n\
  </head>\n\
  <body>\n\
    <h1>404 - Not Found</h1>\n\
  </body>\n\
</html>\n\
]]\n\
\n\
local bodyLen = echo(body)\n\
\n\
local header = header:gsub('\\n', '\\r\\n'):gsub('$bodyLen', bodyLen)\n\
local headerLen = setHeader(header)\n\
\n\
io.write('Lua - '..bodyLen+headerLen..' bytes sent, H:'..headerLen..' & B:'..bodyLen..'\\n')"



#endif
