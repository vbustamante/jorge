local body = [[
<html>
  <head>
    <title>Jorge Root</title>
  </head>
  <body>
    <h1>TONY WORLD!</h1>
  </body>
</html>
]]
local totalLen = echo(body)

io.write("Lua - "..totalLen.." bytes sent\n")
