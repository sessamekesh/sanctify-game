# TypeScript sources (for web frontends)

Monorepo managed with [Lerna](https://github.com/lerna/lerna).

Getting started:

```bash
npm install
```

## Package Overview

### pve-offline-wasm

Library that provides a nice TypeScript interface for dealing with the Sanctify PvE Offline C++ client compiled to WebAssembly.

Be sure to re-build with `lerna run tsc` after modifying.

### pve-offline-client

React app that runs the PvE Offline client

`npm start` in the `packages/pve-offline-client` dir to start this up.
