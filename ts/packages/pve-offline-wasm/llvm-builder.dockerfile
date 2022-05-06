FROM debian:bullseye-20220125

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

WORKDIR /src
