# Revisit this later.
# I would like to have a multi stage build where I get the best setup
# to compile the app with maximal optimizations
# and then run it on a minimal container.
# I want the runtime container to support ftrace and shit so probably
# needs a recent kernel and I dunno alpine seems need because you can get
# a fully static executable and just copy that. None of that matters for
# getting started though so we'll just use ubuntu and build-essential

# @TODO: Also figure out libsodium. Just installing it is probably chill
# for the server. Don't know what to do for linux client yet. TBD

FROM ubuntu:latest

RUN \
  apt-get update && \
  apt-get -y upgrade && \
  apt-get install -y build-essential && \
  apt-get install -y libsodium-dev && \
  apt-get install -y mg && \
  apt-get install -y gdb && \
  rm -rf /var/lib/apt/lists/*

ADD . /root/wizard_server

WORKDIR /root/wizard_server

RUN cc \
    -Wall -g \
    -I3rdparty/include \
    -o wizard_server \
    src/wizard_server.c \
    3rdparty/include/netcode.c \
    -lsodium

EXPOSE 40000

CMD /root/wizard_server/wizard_server
