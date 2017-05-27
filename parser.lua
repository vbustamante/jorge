function parseRequest(request)
  io.write("Request\n")
  request = request:gsub("\r\n", "\\r\\n\n")
  io.write(request)
end
