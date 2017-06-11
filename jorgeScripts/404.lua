local header = [[
HTTP/1.1 404 NOT FOUND
Server: Jorge/0.2
Content-Length: $bodyLen
Content-Type: text/html

]]

local body = [[
<!DOCTYPE html>
<html>
  <head>
    <title>Not Found</title>
  </head>
  <body>
    <h1>404 - Not Found</h1>
  </body>
</html>
]]

local bodyLen = echo(body)

local header = header:gsub('\n', '\r\n'):gsub('$bodyLen', bodyLen)
local headerLen = setHeader(header)

io.write('Lua - '..bodyLen+headerLen..' bytes sent, H:'..headerLen..' & B:'..bodyLen..'\n')