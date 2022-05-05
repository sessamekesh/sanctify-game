# Sanctify Game

> :warning: This project is still under development, and has known build / cross-platform issues.

Browser-based MoBA style game. This repository contains the full monorepo for the entire Sanctify project, including all server binaries, external dependencies, intermediate libraries, development tools, and web applications used for the Sanctify project.

## Local Development

This project uses Docker Compose to make everything easier.

For developing the web frontend:

```bash
docker-compose -f docker-compose.yaml up sanctify-frontend-develop
```

## Dependencies

* Python 3.x
* CMake 3.18+
* A C++ compiler
* NodeJS and NPM

For WebAssembly game builds:

* Emscripten
* Ninja (on Windows)

## Directory Structure

This monorepo hosts several applications across different languages with different build environments. At the top level is a folder for each language/environment used, and inside of those is all the build/project/configuration files used.

- `cpp/` hosts a CMakeLists.txt file and is the root directory for all C++ tools, libraries, external dependencies, and application entry points for C++ projects.
- `ts/` hosts a Lerna monorepo of JavaScript/TypeScript projects.
- `proto/` hosts protocol buffer definition files.

## Project List

For each individual project, go to the appropriate subdirectory for build/running instructions.