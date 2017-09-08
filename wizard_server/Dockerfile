# Revisit this later.
# I would like to have a multi stage build where I get the best setup
# to compile the app with maximal optimizations
# and then run it on a minimal container.
# I want the runtime container to support ftrace and shit so probably
# needs a recent kernel and I dunno alpine seems need because you can get
# a fully static executable and just copy that. None of that matters for
# getting started though so we'll just use ubuntu and build-essential

FROM ubuntu:latest

RUN \
  apt-get update && \
  apt-get -y upgrade && \
  apt-get install -y build-essential && \
  rm -rf /var/lib/apt/lists/*

ADD . /root/wizard_server

WORKDIR /root/wizard_server

RUN cc -Wall src/wizard_server.c -o wizard_server
RUN chmod +x wizard_server
CMD /root/wizard_server/wizard_server
