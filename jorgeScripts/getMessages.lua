local header = [[
HTTP/1.0 $httpCode $httpStatus
Server: Jorge/0.2
Content-Length: $bodyLen
Content-Type: application/json

]]
local bodyLen = 0

local POST = {}

BODY = BODY:gsub ("+", " "):gsub ("%%(%x%x)", -- URL DECODE
  function(h) return string.char(tonumber(h,16)) end)

for key, value in string.gmatch(BODY, '([^&=]+)=([^&=]+)') do
  POST[key] = value
end

local query = "SELECT * FROM messages WHERE id >= $id ORDER BY id ASC LIMIT 50"

if POST["id"] then
  query = query:gsub("$id", POST["id"])
else
  query = query:gsub("$id", "0")
end

ret, err = sqlQuery(query)

if err then
  print('SQL error: '..err)
  header = header:gsub('$httpCode', '500'):gsub('$httpStatus', 'INTERNAL SERVER ERROR')
  
else

  bodyLen = echo('{"messages":[')
  if(ret.count > 0) then
    for i=1, ret.count-1 do
      local data = ret.data[i]
      local sender = data.sender and '"'..data.sender..'"' or 'null'

      bodyLen = bodyLen+echo('{"time":"'..data.time..'","id":"'..data.id..'","msg":"'..data.msg..'","sender":'.. sender ..'},')
    end

    local data = ret.data[ret.count]
    local sender = data.sender and '"'..data.sender..'"' or 'null'

    bodyLen = bodyLen+echo('{"time":"'..data.time..'","id":'..data.id..',"msg":"'..data.msg..'","sender":'..sender..'}]}')

  else
    bodyLen = bodyLen+echo(']}')
  end
  header = header:gsub('$httpCode', '200'):gsub('$httpStatus', 'OK')
end

header = header:gsub('\n', '\r\n'):gsub('$bodyLen', bodyLen)
local headerLen = setHeader(header)

-- print("Lua - "..bodyLen+headerLen.." bytes sent, H:"..headerLen.." & B:"..bodyLen)

