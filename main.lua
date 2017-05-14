io.write("Lua - Calling echo() ...\n")

local body = [[
<html>
  <head>
    <title>Jorge Root</title>
  </head>
  <body>
    <h1>ELLO WORLD!</h1>
  </body>
</html>
]]
local totalLen = echo(body)

io.write(string.format("Lua - %s bytes sent\n", tostring(value)))
