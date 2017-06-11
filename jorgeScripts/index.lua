local header = [[
HTTP/1.1 200 OK
Server: Jorge/0.2
Content-Length: $bodyLen
Content-Type: text/html

]]

local upperBody = [[
<!DOCTYPE html>
<html>
  <head>
    <title>Jorge Root</title>
  </head>
  <body>
    <h1>TONY WORLD!</h1>
]]

local lowerBody = [[
  </body>
</html>
]]



local bodyLen = echo(upperBody)


function tprint (tbl, indent)
  if not indent then indent = 0 end
  for k, v in pairs(tbl) do
    formatting = string.rep("  ", indent) .. k .. ": "
    if type(v) == "table" then
      print(formatting)
      tprint(v, indent+1)
    else
      print(formatting .. v)
    end
  end
end

local ret, err = sqlQuery("INSERT INTO messages(sender, msg) VALUES ('admin', 'Hello3')")
if err then 
  print('SQL error: '..err)
  bodyLen = bodyLen+echo('<h3>SQL error: '..err..'</h3>')  
else

  if ret.count > 0 then print('Got '..ret.count..' rows.') end
  if ret.modified > 0 then print('Created '..ret.modified..' rows.') end
  bodyLen = bodyLen+echo('<h3>modified: '..ret.modified..'</h3>')
  bodyLen = bodyLen+echo('<h3>got: '..ret.count..'</h3><ul>')  
end


ret, err = sqlQuery("SELECT * FROM messages ORDER BY id ASC")

if err then 
  print('SQL error: '..err)
  bodyLen = bodyLen+echo('<h3>SQL error: '..err..'</h3>')  
else

  if ret.count > 0 then print('Got '..ret.count..' rows.') end
  if ret.modified > 0 then print('Created '..ret.modified..' rows.') end
  bodyLen = bodyLen+echo('<h3>modified: '..ret.modified..'</h3>')
  bodyLen = bodyLen+echo('<h3>got: '..ret.count..'</h3><ul>')
  
  for i=1, ret.count do
    local data = ret.data[i]
    print(i..'----------------')
    print(data.time)
    print(data.msg)
    print(data.id)
    print(' ', data.sender)
    print('----------------')
    bodyLen = bodyLen+echo('<li>at '..data.time..':<br>'..data.id..' - '..data.msg..'</li>')

  end
end 

bodyLen = bodyLen+echo('</ul>'..lowerBody)


local header = header:gsub('\n', '\r\n'):gsub('$bodyLen', bodyLen)
local headerLen = setHeader(header)

print("Lua - "..bodyLen+headerLen.." bytes sent, H:"..headerLen.." & B:"..bodyLen)
