#!/bin/bash

set -e

echo "=================================================="
echo "Compiling Indigo toolset"
echo "=================================================="

mkdir -p /build/tools && cd /build/tools
[ ! -f /build/tools/CMakeCache.txt ] && cmake /src/cpp \
          -DIG_BUILD_TESTS="off" -DIG_BUILD_SERVER="off" \
          -DCMAKE_BUILD_TYPE="MinSizeRel" -DIG_ENABLE_THREADS="on" \
          -DIG_CHECK_SUBMODULES_ON_BUILD="off" -DIG_ENABLE_ECS_VALIDATION="off" \
          -DIG_TOOL_WRANGLE_PATH="./igtools.cmake" -G "Ninja"
ninja protoc igpack-gen

echo "=================================================="
echo "Done compiling Indigo toolset"
echo "=================================================="
