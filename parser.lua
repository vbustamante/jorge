function parseRequest(request)
  io.write("Lua parser:\n")
  request = request:gsub("\r\n", "\\r\\n\n")
  --io.write(request)
end
