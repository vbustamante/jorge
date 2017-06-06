# Jorge
## __A lua webserver__

Jorge is an http/https server written in c++ and projected to be a all-in-one web solution for simple projects.
It features a simple path-to-script routing, with support to the lua language and an embedded sqlite database.

It is presently under development and in a very initial state.

### Building Jorge
The project, as well as its dependencies, are all setup to be built by cmake. Everything and the entiry platform is statically compiled as a single file to ensure ease of deployment. 
To build it, you can simply run, in any build folder:
`$ cmake /path/to/jorge/ && make`
And run the executable `Jorge` on the same folder.
