# Sanctify Game Common

Common code shared between the client and server.

Careful! The execution environment of the client (web) and server (linux native) are DRAMATICALLY
different! A handful of assumptions can be made and this is _generally_ safe to do, but take extra
care to test both on native and web environments!

## Copyright Notice

Most code in this library is licensed under the same license of the broader Sanctify project, though
this library does contain some exceptions - copyright notices are included in all affected source
files.

Some code in sanctify-game-common/net (specifically - reliable.h and reliable.cc) is taken from the
fantastic work done by Glenn Fiedler in the
[Gaffer On Games](https://gafferongames.com/post/reliable_ordered_messages/) article about
building a pseudo-reliable network transport layer over UDP. Code is adapted directly from the
[networkprotocol/reliable](https://github.com/networkprotocol/reliable) code, and is is ported
from C directly into WASM/WebRTC compatible C++ being as faithful to the original as possible.
This code is licensed under the [BSD 3-Clause license](https://opensource.org/licenses/BSD-3-Clause)