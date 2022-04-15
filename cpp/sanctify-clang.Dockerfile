FROM debian:bullseye

RUN apt-get -qq update
RUN apt-get -qq install -y cmake git make lsb-release
RUN apt-get -qq install -y wget software-properties-common
RUN apt-get -qq install -y gnupg2 xorg-dev libx11-xcb-dev

WORKDIR /usr/deps

RUN wget https://apt.llvm.org/llvm.sh
RUN chmod +x llvm.sh
RUN ./llvm.sh 14

RUN apt-get -qq install libssl-dev libboost-all-dev

ENV CC=/usr/bin/clang-14
ENV CXX=/usr/bin/clang++-14
ENV CPP=/usr/bin/clang-cpp-14
ENV LD=/usr/bin/ld.lld-14

CMD ["sleep", "infinity"]
