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

## Simulation Core

Generally speaking, components and systems will serve some purpose:

* (A) Updating the authoritative server state
* (B) Performing calculations required to generate the render state of the world

Most components and systems that are part of (A) will also be part of (B), with some exceptions:

* Systems that generate more information than is needed for the client may be included only in (A).
  * Pathfinding - the client shouldn't know the full path of any enemy, and only needs to know the
    final path that the client player will take.
* Systems that produce actions based on player input
  * Player ability activation systems - they fire an event that may be picked up by both client and
    server systems, but the client should not attempt to preempt RTT by firing events immediately.

The intersection of (A) and (B) are included in this library to facilitate code sharing.

## Network Synchronization

_TODO (sessamekesh): Outline network synchronization design in a Google Doc or something._

### Server Side

1) Update the world state every game tick
2) Enumerate which clients need to receive a new world state
3) For each client which needs an update...
    * Generate a serialized world snapshot that represents the authoritative game state
    * Generate a compressed diff based on the last snapshot the client is known to have
    * Send the client the diff, along with the current authoritative server time of that diff
    * Store the diff for later snapshot reconstruction

### Client Side

0) Establish an estimate for the _perceived_ server time - the simulation runs off that clock
1) Update the local world state every game tick
2) Unpack the last diff received from the server, and combine it with the last known snapshot
   to generate an authoritative server state
3) Generate a world state based on that server snapshot
4) Advance the world state to the current client-side clock time
5) Generate snapshots of both the client simulation, and (time-advanced) server simulation
6) Generate a diff from client->server snapshot
7) Reconcile the diff

### Shared Code

Basically, both the client and server have strong need to be able to construct a snapshot of
world state given various input entities / components, efficiently construct diffs between two
snapshots, apply a diff _to_ a snapshot, and read/write their state to/from a game world.

### Updating Snapshot code to transfer new component types

BE VERY CAREFUL as this is something that can be highly error prone!

TODO (sessamekesh): work on a less error prone verison of this (unholy macros might be better than this shit)

The following places need to be updated if there is any new net sync components added:

1) sanctify-net.proto :: ComponentData+GameEntityUpdateMask::ComponentType needs a new field for the new component
2) game_snapshot.cc :: GameSnapshotDiff::serialize needs to add diff vals to proto
3) game_snapshot.cc :: GameSnapshotDiff::deserialize needs to extract diff values from proto
4) game_snapshot.cc :: GameSnapshot::serialize needs to add diff vals to proto
5) game_snapshot.cc :: GameSnapshot::deserialize needs to extract diff values from proto
6) game_snapshot_test.cc :: GameSnapshotDiff TestSerializeAndDeserialize test
    * Under section "ADD COMPONENT DELETIONS" and "VERIFY COMPONENT DELETIONS"
    * Under section "ADD COMPONENTS" and "VERIFY NEW COMPONENTS"
7) game_snapshot_test.cc :: GameSnapshot TestSerializeAndDeserialize test
    * Under section "ADD COMPONENTS" and "VERIFY COMPONENTS"
    * Under section "VERIFY COMPONENT DELETIONS"
8) entt_snapshot_translator.cc :: write_fresh_game_state (write a component if it's found in the snapshot)
9) entt_snapshot_translator.cc :: read_all_game_state (read a component if it's found in the world)
10) reconcile_net_state_system.cc :: reconcile_client_state (removing components from a world state)
11) net_serialize.cc :: gen_snapshot_for_players

Also make sure that the following are true:
1) The new component type has the equality operator overloaded
2) The new component type has a working copy constructor
3) FOR GOODNESS SAKE the component must ONLY translate information that will make sense to a fresh world state!!!!
    * Pointers in your component types are a DEFINITE NO-NO
    * Opaque handles are okay if and only if the server and client have a separate mechanism for keeping them in sync, and/or the client can behave gracefully in the case that the opaque handles are invalid.

There's a bunch of helpers that are useful in game_snapshot.cc that can help (::extract_<<component_type>> etc)