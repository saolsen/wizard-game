Build Server V1

Server runs.
Client can connect to server.
Client can send a msgpack message.
Server relays all messages to all connected clients (and says their client id).

Get that running on all platforms (at least linux and windows)

Unit test program that tests stuff.
Integration test program / script that has some clients connect to a server and echo messages.

All of that building / testing in CI.

Real v1
Connect to server.
Say your name.
Get spawned in the world.
Can walk around and see other people walking around.
That's a good foundation for shooting fireballs and stuff.


How do I want to manage dependencies?
- git submodules, build everything from scratch?
- fully embed all projects, build everything from scratch?
- build libs manually, embed compiled libs?

I kinda really like the submodule and everything from scratch idea. Premake makes it possible to really handle that.
maybe another premake file, build-deps that just takes care of everything for every platform and makes an include and lib folder.


Lets do this
- get that poc and test stuff running on windows (and server linux with current dockerfile)

THEN worry about deps and builds.
- submodules for any build from source things.
- release archives for stuff you download as a release and compile (like libsodium and prolly glfw3)
- just check in any windows libs
- have a script for compiling dependencies that need compilation.