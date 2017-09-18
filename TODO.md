Build Server V1

Connect to server.
Say your name.
Get spawned in the world.
Can walk around and see other people walking around.
That's a good foundation for shooting fireballs and stuff.

Right now
- pass messages (no reliability yet but whatever)
    - write the game logic with messages (all just assumed reliable (even tho they really aint))


How does the game work?
- clients send input. (reliably I think, or maybe not reliably but each message is their input over the last few frames or something)
- server sends state updates.

Simulation runs on server and clients, server is authoritative. So if a client says they hit somebody server can be like nah.
Client needs to interpolate quickly betweeen their current state and the server state when they get out of sync.

my task for this week is figuring out this stuff for simple wizard game. Will just use udp and pretend everything is reliable even though it's not true and figure that out later.

there's player input
server update
a simulation with logic that runs on both sides
how do you manage all that?
how does time work?
- I think reliable will give you some shit that useful to help out.