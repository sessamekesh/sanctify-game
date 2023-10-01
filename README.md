# Sanctify Game

> **Warning**
> For the time being, I've stopped working on this project, but I've kept it here as a reference (mostly for myself, I'm sure!)
> I don't see myself ever getting back to it, and I might take it down if I end up publishing the source code for other game(s) I've
> built that use this technology as a base.

> **Warning**
> This project is still under development, and has known build / cross-platform issues.
> I'm publishing it as-is because it does demonstrate a lot of cool stuff around game programming with WebGPU.

Browser-based MoBA style game. This repository contains the full monorepo for the entire Sanctify project, including all server binaries, external dependencies, intermediate libraries, development tools, and web applications used for the Sanctify project.

## Local Development

This project uses Docker Compose to make everything easier.

For developing the web frontend:

```bash
docker-compose -f docker-compose.yaml up sanctify-frontend-develop
```

## Dependencies

* NodeJS and NPM
* Docker

For local development of WASM code:

* CMake 3.18+
* A C++ compiler

## Directory Structure

This monorepo hosts several applications across different languages with different build environments. At the top level is a folder for each language/environment used, and inside of those is all the build/project/configuration files used.

- `cpp/` hosts a CMakeLists.txt file and is the root directory for all C++ tools, libraries, external dependencies, and application entry points for C++ projects.
- `ts/` hosts a Lerna monorepo of JavaScript/TypeScript projects.
- `proto/` hosts protocol buffer definition files.

## Project List

For each individual project, go to the appropriate subdirectory for build/running instructions.

## [WIP] Building the web app

1) First, all Git submodules need to be pulled. To help with build times, this is not done automatically in CMake runs by default.

```bash
# From root directory
git submodule init
git submodule update
```

2) Second, Docker images for performing WASM builds must be built

```bash
cd ts/packages/pve-offline-wasm
docker build -f emscripten-builder.dockerfile -t sanctify-emsdk .
docker build -f llvm-builder.dockerfile -t sanctify-llvm .
```

3) WASM builds can no be performed as regular

```bash
cd ts
# TODO (sessamekesh): This command only needs to be run between builds?
yarn build --force
yarn dev
```
