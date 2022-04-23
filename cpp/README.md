# Dependencies

_General Dependencies_
* A C++ compiler
* `cmake git make lsb-release`

_Server builds_
* `boost`

_Client builds_
* `xorg-dev libx11-xcb-dev`

_WebAssembly builds_
* Emscripten 3.0.1+

# Submodules

Some external dependencies are kept in Git submodules, and those dependencies keep other submodules.

You can either...
- (1) Update submodules yourself with `git submodule update --init --recursive` after cloning/changing submodules
- (2) Run CMake with argument "-DIG_CHECK_SUBMODULES_ON_BUILD=ON"

# C++ Sources Root

- `/cmake`: CMake scripts with functions, macros, and external dependency wrangling.
- `/extern`: External dependencies that cannot be wrangled with CMake's `FetchContent` module
- `/libs`: Indigo libraries (copied around from project to project because they're great)
- `/tools`: Source code for offline build tools used to generate asset files, etc.
- `/sanctify-game/common`: Common code shared between the sanctify game server and client (main game logic)
- `/sanctify-game/server`: Source code for sanctify game server executable
- `/sanctify-game/client`: Source code for Sanctify game client library (including entry point for native executable and EMBINDs for web module)

# Local verify notes
```bash
# Build image (only needs to be done once)
docker build -f ./sanctify-clang.Dockerfile -t sanctify-clang .

# Start the container (replacing K:/games/sanctify-game with your absolute git path)
docker run -v K:/games/sanctify-game:/usr/src/sanctify-game sanctify-clang

# Build and run DEBUG tests (useful for checking dev asserts)
# (I usually do this by opening up a new CLI and attaching to the running Docker image)
mkdir -p /usr/build/sanctify-game/dbg && cd /usr/build/sanctify-game/dbg
cmake /usr/src/sanctify-game/cpp -DIG_BUILD_TESTS="on" -DIG_BUILD_SERVER="on" -DCMAKE_BUILD_TYPE="Debug" -DIG_ENABLE_THREADS="on" -DIG_CHECK_SUBMODULES_ON_BUILD="ON" -DIG_ENABLE_ECS_VALIDATION="ON" -DIG_TOOL_WRANGLE_PATH="./igtools.cmake"
make igcore_test igecs-test
./libs/igcore/igcore_test
./libs/igecs/igecs-test
# Or, alternatively: just make and ctest

# Build and run PROD tests (useful for... checking prod builds)
mkdir -p /usr/build/sanctify-game/rel && cd /usr/build/sanctify-game/rel
cmake /usr/src/sanctify-game/cpp -DIG_BUILD_TESTS="on" -DIG_BUILD_SERVER="on" -DCMAKE_BUILD_TYPE="MinSizeRel" -DIG_ENABLE_THREADS="on" -DIG_CHECK_SUBMODULES_ON_BUILD="ON" -DIG_ENABLE_ECS_VALIDATION="OFF" -DIG_TOOL_WRANGLE_PATH="./igtools.cmake"
make igcore_test igecs-test
./libs/igcore/igcore_test
./libs/igecs/igecs-test
```

Building the JavaScript

```bash
# Build clang image (only needs to be done once)
docker build -f ./sanctify-clang.Dockerfile -t sanctify-clang .

# Configure and build tools
docker run --rm -v K:/games/sanctify-game:/usr/src/sanctify-game -w /usr/src/sanctify-game/cpp/out/emcc-tools sanctify-clang cmake /usr/src/sanctify-game/cpp -DIG_BUILD_TESTS="on" -DIG_BUILD_SERVER="on" -DCMAKE_BUILD_TYPE="MinSizeRel" -DIG_ENABLE_THREADS="on" -DIG_CHECK_SUBMODULES_ON_BUILD="ON" -DIG_ENABLE_ECS_VALIDATION="ON" -DIG_TOOL_WRANGLE_PATH="./igtools.cmake"
docker run --rm -v K:/games/sanctify-game:/usr/src/sanctify-game -w /usr/src/sanctify-game/cpp/out/emcc-tools sanctify-clang make protoc igpack-gen

# Build emscripten image (only needs to be done once)
docker build -f ./sanctify-emscripten.Dockerfile -t sanctify-emscripten .

# Configure and build binaries for multi-threaded (standard) target
docker run --rm -v K:/games/sanctify-game:/usr/src/sanctify-game -w /usr/src/sanctify-game/cpp/out/emcc sanctify-emscripten emcmake cmake /usr/src/sanctify-game/cpp -DIG_BUILD_TESTS="OFF" -DIG_BUILD_SERVER="off" -DCMAKE_BUILD_TYPE="MinSizeRel" -DIG_ENABLE_THREADS="on" -DIG_CHECK_SUBMODULES_ON_BUILD="ON" -DIG_ENABLE_ECS_VALIDATION="off" -DIG_TOOL_WRANGLE_PATH="../emcc-tools/igtools.cmake"
docker run --rm -v K:/games/sanctify-game:/usr/src/sanctify-game -w /usr/src/sanctify-game/cpp/out/emcc sanctify-emscripten emmake make sanctify-pve-offline-client

# Configure and build binaries for single-threaded target
docker run --rm -v K:/games/sanctify-game:/usr/src/sanctify-game -w /usr/src/sanctify-game/cpp/out/emcc-st sanctify-emscripten emcmake cmake /usr/src/sanctify-game/cpp -DIG_BUILD_TESTS="OFF" -DIG_BUILD_SERVER="off" -DCMAKE_BUILD_TYPE="MinSizeRel" -DIG_ENABLE_THREADS="off" -DIG_CHECK_SUBMODULES_ON_BUILD="ON" -DIG_ENABLE_ECS_VALIDATION="off" -DIG_TOOL_WRANGLE_PATH="../emcc-tools/igtools.cmake"
docker run --rm -v K:/games/sanctify-game:/usr/src/sanctify-game -w /usr/src/sanctify-game/cpp/out/emcc-st sanctify-emscripten emmake make sanctify-pve-offline-client

# ... And don't forget to copy them over to the web entry point
cp K:/games/sanctify-game/cpp/out/emcc/sanctify/pve/offline_client/sanctify-pve-offline-client.js K:/games/sanctify-game/ts/packages/pve-offline-client/public/wasm_mt/sanctify-pve-offline-client.js
cp K:/games/sanctify-game/cpp/out/emcc/sanctify/pve/offline_client/sanctify-pve-offline-client.worker.js K:/games/sanctify-game/ts/packages/pve-offline-client/public/wasm_mt/sanctify-pve-offline-client.worker.js
cp K:/games/sanctify-game/cpp/out/emcc/sanctify/pve/offline_client/sanctify-pve-offline-client.wasm K:/games/sanctify-game/ts/packages/pve-offline-client/public/wasm_mt/sanctify-pve-offline-client.wasm

cp K:/games/sanctify-game/cpp/out/emcc-st/sanctify/pve/offline_client/sanctify-pve-offline-client.js K:/games/sanctify-game/ts/packages/pve-offline-client/public/wasm_st/sanctify-pve-offline-client.js
cp K:/games/sanctify-game/cpp/out/emcc-st/sanctify/pve/offline_client/sanctify-pve-offline-client.wasm K:/games/sanctify-game/ts/packages/pve-offline-client/public/wasm_st/sanctify-pve-offline-client.wasm
```

# General Development Rules
* Use LibECS for everything that doesn't happen on the main thread.
