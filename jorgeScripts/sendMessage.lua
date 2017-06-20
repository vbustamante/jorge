local header = [[
HTTP/1.1 200 OK
Server: Jorge/0.2
Content-Length: 0
Content-Type: text/html

]]

for pair in string.gmatch(BODY, '([^\n]+)') do
    print(pair)
end
--for pair in split(BODY, "\n") do
--    local t = split(pair, '=')
--    print("Item")
--    print(t[1])
--    print(t[2])
--end

ret, err = sqlQuery("SELECT * FROM messages ORDER BY id ASC")


local header = header:gsub('\n', '\r\n')
setHeader(header)

