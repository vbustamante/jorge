
io.write(string.format("Ello from %s\n", _VERSION))

io.write("Calling echo() ...\n")
local value = echo("First", "Second", 112233)
io.write(string.format("echo() returned: %s\n", tostring(value)))
