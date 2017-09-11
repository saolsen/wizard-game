# Dockerfile for the server.
# For dev and test not prod.
FROM ubuntu:devel

RUN \
  apt-get update && \
  apt-get -y upgrade && \
  apt-get install -y build-essential && \
  apt-get install -y mg && \
  apt-get install -y gdb && \
  apt-get install -y netcat && \
  apt-get install -y strace && \
  apt-get install -y tcpdump && \
  apt-get install -y tmux && \
  apt-get install -y net-tools && \
  apt-get install -y wget && \
  rm -rf /var/lib/apt/lists/*

RUN wget https://github.com/jedisct1/libsodium/releases/download/1.0.10/libsodium-1.0.10.tar.gz && \
    tar -zxvf libsodium-1.0.10.tar.gz && \
    cd libsodium-1.0.10 && \
    ./configure && \
    make && make check && \
    make install && \
    cd .. && \
    rm -rf libsodium* && \
    ldconfig

RUN wget https://github.com/premake/premake-core/releases/download/v5.0.0-alpha10/premake-5.0.0-alpha10-linux.tar.gz && \ 
    tar -zxvf premake-*.tar.gz && \
    rm premake-*.tar.gz && \
    mv premake5 /usr/local/bin

ADD . /root/wizard-game
WORKDIR /root/wizard-game

# @TODO: Build everything.
RUN find . -exec touch {} \; && premake5 gmake && cd build && make -j32 WizardServer config=release

EXPOSE 40000