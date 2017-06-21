local header = [[
HTTP/1.0 $httpCode $httpStatus
Server: Jorge/0.2
Content-Length: 0
Content-Type: text/html

]]
local POST = {}

BODY = BODY:gsub ("+", " "):gsub ("%%(%x%x)", -- URL DECODE
    function(h) return string.char(tonumber(h,16)) end)

for key, value in string.gmatch(BODY, '([^&=]+)=([^&=]+)') do
    POST[key] = value
end

if (not POST["msg"]) then
    header = header:gsub('$httpCode', '400'):gsub('$httpStatus', 'Bad Request')
else
    local query = "INSERT INTO messages(msg, sender) VALUES('$msg', '$sender')"
    local escapedMsg = ""
    for i in POST["msg"]:gmatch( "%C+" ) do
        escapedMsg = escapedMsg .. i
    end
    query = query:gsub("$msg", escapedMsg)
    if POST["sender"] then
        query = query:gsub("$sender", POST["sender"])
    else
        query = query:gsub("$sender", "NULL")
    end
    local ret, err = sqlQuery(query)

    if err then
        print(err)
        header = header:gsub('$httpCode', '503'):gsub('$httpStatus', 'INTERNAL SERVER ERROR')
    else
        header = header:gsub('$httpCode', '200'):gsub('$httpStatus', 'OK')
    end
end

local header = header:gsub('\n', '\r\n')
setHeader(header)

