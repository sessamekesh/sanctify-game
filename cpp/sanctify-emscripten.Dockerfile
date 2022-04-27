FROM debian:bullseye

RUN apt-get -qq update
RUN apt-get -qq install -y cmake git make lsb-release wget software-properties-common gnupg2
RUN apt-get -qq install -y python3 lbzip2 ninja-build

WORKDIR /usr/deps

RUN wget https://apt.llvm.org/llvm.sh
RUN chmod +x llvm.sh
RUN ./llvm.sh 14

RUN apt-get -qq install libssl-dev libboost-all-dev

ENV CC=/usr/bin/clang-14
ENV CXX=/usr/bin/clang++-14
ENV CPP=/usr/bin/clang-cpp-14
ENV LD=/usr/bin/ld.lld-14

WORKDIR /usr/deps

RUN git clone https://github.com/emscripten-core/emsdk.git
WORKDIR /usr/deps/emsdk

RUN ./emsdk install latest
RUN ./emsdk activate latest

ENV PATH="$PATH:/usr/deps/emsdk:/usr/deps/emsdk/upstream/emscripten:/usr/deps/emsdk/upstream/bin:/usr/deps/emsdk/node/14.18.2_64bit/bin"
ENV EMSDK=/usr/deps/emsdk
ENV EM_CONFIG=/usr/deps/emsdk/.emscripten
ENV EMSDK_NODE=/usr/deps/emsdk/node/14.18.2_64bit/bin/node
ENV EMSCRIPTEN=/usr/deps/emsdk/upstream/emscripten
