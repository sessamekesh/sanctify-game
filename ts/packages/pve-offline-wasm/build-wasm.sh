#!/bin/bash

set -e

echo "=================================================="
echo "Compiling PVE Offline WASM bindings"
echo "=================================================="

mkdir -p /build/wasm-st && cd /build/wasm-st
[ ! -f /build/wasm-st/CMakeCache.txt ] && emcmake cmake /src/cpp \
          -DIG_BUILD_TESTS="off" -DIG_BUILD_SERVER="off" \
          -DCMAKE_BUILD_TYPE="MinSizeRel" -DIG_ENABLE_THREADS="off" \
          -DIG_CHECK_SUBMODULES_ON_BUILD="off" -DIG_ENABLE_ECS_VALIDATION="off" \
          -DIG_TOOL_WRANGLE_PATH="/build/tools/igtools.cmake" -G "Ninja"
emmake ninja sanctify-pve-offline-client

mkdir -p /build/wasm-mt && cd /build/wasm-mt
[ ! -f /build/wasm-mt/CMakeCache.txt ] && emcmake cmake /src/cpp \
          -DIG_BUILD_TESTS="off" -DIG_BUILD_SERVER="off" \
          -DCMAKE_BUILD_TYPE="MinSizeRel" -DIG_ENABLE_THREADS="on" \
          -DIG_CHECK_SUBMODULES_ON_BUILD="off" -DIG_ENABLE_ECS_VALIDATION="off" \
          -DIG_TOOL_WRANGLE_PATH="/build/tools/igtools.cmake" -G "Ninja"
emmake ninja sanctify-pve-offline-client

mkdir -p /out/wasm_mt
mkdir -p /out/wasm_st
mkdir -p /out/resources

cp /build/wasm-st/sanctify/pve/offline_client/sanctify-pve-offline-client.js /out/wasm_st
cp /build/wasm-st/sanctify/pve/offline_client/sanctify-pve-offline-client.wasm /out/wasm_st
cp /build/wasm-mt/sanctify/pve/offline_client/sanctify-pve-offline-client.js /out/wasm_mt
cp /build/wasm-mt/sanctify/pve/offline_client/sanctify-pve-offline-client.worker.js /out/wasm_mt
cp /build/wasm-mt/sanctify/pve/offline_client/sanctify-pve-offline-client.wasm /out/wasm_mt
cp /build/wasm-mt/sanctify/pve/offline_client/resources/* /out/resources

echo "=================================================="
echo "Done compiling PVE Offline WASM bindings"
echo "=================================================="
