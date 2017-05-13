io.write("Lua - Calling echo() ...\n")

local value = echo("Firste", "Second\n", 112233, "end")
value = value + echo("Firste", "SecondRUN\n", "end for real this time")

io.write(string.format("Lua - %s bytes sent\n", tostring(value)))
