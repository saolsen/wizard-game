# Wizard Game

Wizard game is a multiplayer battleground game where wizards battle.

## Getting Started

There are a few components.

* WizardClient - An app that runs on players computers. This is the game that you download.
* WizardServer - A service that the clients connect to and manages a running instance of the game.
* WizardAPI - A web service that manages authentication, matchmaking and managing WizardServer instances.

WizardClient is written in C (and uses raylib). The client is targeted at the big three desktop platforms; Windows, OSX and linux. (They have not all been tested yet.)

WizardServer is also written in c and is a netcode.io service. It runs on Windows and OSX because it makes development easier but it is really only meant to target Linux in production.

WizardAPI is written in python and uses postgres to store state.

### Prerequisites

For wizard client and server you need to have premake5 installed to generate project files and build the project. Dependencies vary by platform.

#### Windows
* Visual Studio 2017
#### OSX
* Xcode command line tools (basically just clang and make)
#### Linux
* A C compiler
* make
* libsodium

For WizardAPI you need python3 and to install the requirements in the requirements.txt in the web folder. You'll also need postgres to run it and the easiest way to get that all setup is to just use the included Dockerfile and docker-compose files. See below.

### Building

#### Windows

Run `premake5 solution` to generate project files and open the solution. Then use visual studio to build the individual projects.

#### Linux

Run `premake5 gmake` to generate a makefile, then run the individual targets with make.

There are also premake5 targets to build and test programs directly.

@TODO: None of these are real yet.
* `premake5 server` Builds and runs the server with no web component.
* `premake5 client` Builds and runs a client that can connect directly to the server running on localhost.
* `premake5 docker` Spins up the whole stack locally (including launching game servers) with docker and docker-compose.
* `premake5 unit-test` Builds and runs unit tests. This can be run on every platform. Can prolly pass some args on how serious to do property tests.
* probably want a more serious "unit test" that uses real packets and a simulated network with packet loss and stuff to check properties. Dunno if that's doable in a cross platform binary like unit-test or if it would take some linux specific stuff.
* `premake5 integration-test` Spins up the whole stack with docker, runs some simulated clients and tests the full stack.
* `premake5 security-test` Some clang and memcheck and stuff, probably run in docker.
* `premake5 perf-test` Prolly some docker stuff too. Hopefully published on each commit by CI.
* `premake5 release-client` Builds a full optimized app bundle for the platform it's run on.

@TODO: You can't just build a release for the server probably. We'll probably use either a multi-stage docker build or packer to build an AMI or something. There's no real release of the server or web without all the stuff that goes with running it in prod like monitoring / logging / etc.

### Development

After generating platform specific project files you can work on the client and server locally and by default they'll connect to hardcoded local ports that match up and skip any of the matchmaking. If you want the full stack use docker.

### Tests

Tests are run like this. There are a bunch. @TODOsx

## Deployment / Releasing

@TODO: something like.
The clients are built in release mode and bundled into platform specific bundles like this. Our CI system builds new tags on all three platforms and uploads to here.

The web and server stuff gets deployed like this. CI takes care of deploying to these two environments. Here is how config stuff is managed. Blah.

## Built With

Wizard Game uses a few wonderful open source libraries.

* [netcode.io](https://github.com/networkprotocol/netcode.io)
* [realiable.io](https://github.com/networkprotocol/reliable.io)
* [raylib](https://github.com/raysan5/raylib)
