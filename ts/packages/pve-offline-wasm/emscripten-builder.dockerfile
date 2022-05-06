FROM debian:bullseye-20220125 AS sanctify-wasm-deps

RUN apt-get -qq update
RUN apt-get -qq install -y cmake git make lsb-release wget software-properties-common \
            python3 lbzip2 ninja-build libssl-dev libboost-all-dev gnupg2 xorg-dev \
            libx11-xcb-dev rsync

WORKDIR /usr/deps

RUN wget https://apt.llvm.org/llvm.sh
RUN chmod +x llvm.sh
RUN ./llvm.sh 14

ENV CC=/usr/bin/clang-14
ENV CXX=/usr/bin/clang++-14
ENV CPP=/usr/bin/clang-cpp-14
ENV LD=/usr/bin/ld.lld-14

WORKDIR /usr/deps

RUN git clone https://github.com/emscripten-core/emsdk.git
WORKDIR /usr/deps/emsdk
RUN ./emsdk install latest
RUN ./emsdk activate latest

RUN mkdir -p /sanctify/cc && mkdir -p /sanctify/proto && mkdir -p /sanctify/assets

ENV PATH="$PATH:/usr/deps/emsdk:/usr/deps/emsdk/upstream/emscripten:/usr/deps/emsdk/upstream/bin:/usr/deps/emsdk/node/14.18.2_64bit/bin"
ENV EMSDK=/usr/deps/emsdk
ENV EM_CONFIG=/usr/deps/emsdk/.emscripten
ENV EMSDK_NODE=/usr/deps/emsdk/node/14.18.2_64bit/bin/node
ENV EMSCRIPTEN=/usr/deps/emsdk/upstream/emscripten

WORKDIR /src
