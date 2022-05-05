# Thank you https://github.com/belgattitude/nextjs-monorepo-example!

# TODO (sessamekesh): Apply monolith build strategy from
#  https://cjolowicz.github.io/posts/incremental-docker-builds-for-monolithic-codebases/
#  to get cmake incremental builds working (instead of re-building everything!)

#
# Stage 1: Install all workspaces (dev)dependencies and generate node_modules folder(s)
#
FROM node:16-alpine3.15 AS ts-deps
RUN apk add --no-cache rsync

WORKDIR /workspace-install

COPY ts/yarn.lock ./

RUN --mount=type=bind,source=ts,target=/docker-context \
    rsync -amv -delete \
          --exclude='node_modules' \
          --exclude='*/node_modules' \
          --include='package.json' \
          --include='*/' --exclude='*' \
        /docker-context/ /workspace-install/

RUN --mount=type=cache,target=/root/.yarn3-cache,id=yarn3-cache \
    YARN_CACHE_FOLDER=/root/.yarn3-cache \
    yarn install --immutable --inline-builds

#
# Stage 2: Native tool builds (protoc + igpack-gen, need native binaries)
#
FROM debian:bullseye-20220125 AS sanctify-tool-deps

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

RUN mkdir -p /sanctify/cc && mkdir -p /sanctify/proto && mkdir -p /sanctify/assets

WORKDIR /sanctify/tools

RUN --mount=type=bind,source=cpp,target=/sanctify/cc \
    --mount=type=bind,source=proto,target=/sanctify/proto \
    --mount=type=bind,source=assets,target=/sanctify/assets \
    cmake /sanctify/cc \
            -DIG_BUILD_TESTS="off" -DIG_BUILD_SERVER="off" \
            -DCMAKE_BUILD_TYPE="MinSizeRel" -DIG_ENABLE_THREADS="on" \
            -DIG_CHECK_SUBMODULES_ON_BUILD="ON" -DIG_ENABLE_ECS_VALIDATION="off" \
            -DIG_TOOL_WRANGLE_PATH="./igtools.cmake" -G "Ninja" &&\
    ninja protoc igpack-gen


#
# Stage 3: WASM binary builds (sanctify-pve-offline-target)
#
FROM debian:bullseye-20220125 AS sanctify-wasm-deps

RUN apt-get -qq update
RUN apt-get -qq install -y cmake git make lsb-release wget software-properties-common \
            python3 lbzip2 ninja-build libssl-dev libboost-all-dev gnupg2 xorg-dev \
            libx11-xcb-dev rsync

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

COPY --from=sanctify-tool-deps /sanctify/tools /sanctify/tools

WORKDIR /sanctify/emcc-mt-bin
RUN --mount=type=bind,source=cpp,target=/sanctify/cc \
    --mount=type=bind,source=proto,target=/sanctify/proto \
    --mount=type=bind,source=assets,target=/sanctify/assets \
    emcmake cmake /sanctify/cc \
            -DIG_BUILD_TESTS="off" -DIG_BUILD_SERVER="off" \
            -DCMAKE_BUILD_TYPE="MinSizeRel" -DIG_ENABLE_THREADS="on" \
            -DIG_CHECK_SUBMODULES_ON_BUILD="ON" -DIG_ENABLE_ECS_VALIDATION="off" \
            -DIG_TOOL_WRANGLE_PATH="/sanctify/tools/igtools.cmake" -G "Ninja" &&\
    emmake ninja sanctify-pve-offline-client

WORKDIR /sanctify/emcc-st-bin
RUN --mount=type=bind,source=cpp,target=/sanctify/cc \
    --mount=type=bind,source=proto,target=/sanctify/proto \
    --mount=type=bind,source=assets,target=/sanctify/assets \
    emcmake cmake /sanctify/cc \
            -DIG_BUILD_TESTS="off" -DIG_BUILD_SERVER="off" \
            -DCMAKE_BUILD_TYPE="MinSizeRel" -DIG_ENABLE_THREADS="off" \
            -DIG_CHECK_SUBMODULES_ON_BUILD="ON" -DIG_ENABLE_ECS_VALIDATION="off" \
            -DIG_TOOL_WRANGLE_PATH="/sanctify/tools/igtools.cmake" -G "Ninja" &&\
    emmake ninja sanctify-pve-offline-client

#
# Optional: develop web frontend
#
FROM node:16-alpine3.15 AS develop
ENV NODE_ENV=development

WORKDIR /app

# TS
COPY --from=ts-deps /workspace-install ./

# WASM binaries
COPY --from=sanctify-wasm-deps /sanctify/emcc-mt-bin/sanctify/pve/offline_client/sanctify-pve-offline-client.js /app/webmain/public/wasm_mt/
COPY --from=sanctify-wasm-deps /sanctify/emcc-mt-bin/sanctify/pve/offline_client/sanctify-pve-offline-client.wasm /app/webmain/public/wasm_mt/
COPY --from=sanctify-wasm-deps /sanctify/emcc-mt-bin/sanctify/pve/offline_client/sanctify-pve-offline-client.worker.js /app/webmain/public/wasm_mt/
COPY --from=sanctify-wasm-deps /sanctify/emcc-st-bin/sanctify/pve/offline_client/sanctify-pve-offline-client.js /app/webmain/public/wasm_st/
COPY --from=sanctify-wasm-deps /sanctify/emcc-st-bin/sanctify/pve/offline_client/sanctify-pve-offline-client.wasm /app/webmain/public/wasm_st/

# Game resources (generated in same pipeline as WASM binaries)
COPY --from=sanctify-wasm-deps /sanctify/emcc-st-bin/sanctify/pve/offline_client/resources/common-shaders.igpack /app/webmain/public/resources/

EXPOSE 3000

WORKDIR /app/webmain
CMD ["yarn", "dev"]
