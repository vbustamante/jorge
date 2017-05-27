local header = [[
HTTP/1.1 200 OK
Server: Jorge/0.2
Content-Length: $bodyLen
Content-Type: text/html

]]

local body = [[
<!DOCTYPE html>
<html>
  <head>
    <title>Jorge Root</title>
  </head>
  <body>
    <h1>TONY WORLD!</h1>
  </body>
</html>
]]

local bodyLen = echo(body)

header:gsub("\n", "\r\n"):gsub("${bodyLen}", bodyLen)
local headerLen = setHeader(header)

io.write("Lua - "..bodyLen+headerLen.." bytes sent, H:"..headerLen.." & B:"..bodyLen.."\n")
