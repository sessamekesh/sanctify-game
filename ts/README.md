# TypeScript sources (for web frontends)

Monorepo managed with [Lerna](https://github.com/lerna/lerna).

Getting started:

```bash
yarn install
```

## Package Overview

### webmain

Package for the main Sanctify website (Next JS app).

### wasm-wrappers/pve-offline

Library that provides a nice TypeScript interface for dealing with the Sanctify PvE Offline C++ client compiled to WebAssembly.

```bash
yarn proto:generate
yarn tsc
```
