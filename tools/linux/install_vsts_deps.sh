#!/bin/bash

apt-get update && apt-get install -y musl-tools

RUN wget https://github.com/jedisct1/libsodium/releases/download/1.0.10/libsodium-1.0.10.tar.gz && \
    tar -zxvf libsodium-1.0.10.tar.gz && \
    cd libsodium-1.0.10 && \
    ./configure && \
    make && make check && \
    make install && \
    cd .. && \
    rm -rf libsodium* && \
    ldconfig /
