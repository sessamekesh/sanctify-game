# Net Sync (common)

The classes in this folder are related to synchronizing a simulation between a client
and a server via messages that can be exchanged over the network

Every entity that has a "NetSyncId" component attached to it will be considered for
capture in the client/server synchronization. The details of constructing or applying
a snapshot or a diff are left to the application, to allow for custom logic.

## NetSyncId

Any object that should be synchronized between a client and a server should have a
NetSyncId component attached to it. This number is kept in sync between the client
and server, and identifies the object.

Generally speaking, the server should create synchronized entities and attach the
identifier - the client should not be creating new entities.

## General Synchronization Flow

### Server flow

Server takes client input received since the last tick and attaches server specific
components.

Server runs tick logic - this can include shared and server-specific logic.

At the bottom of the frame, if the server decides a client needs a new snapshot, it
creates a snapshot object. This snapshot should contain a CommonLogicSnapshot object.

This snapshot is populated with everything the server decides the client should know
about - this should NOT be the full world state, but a view that the server decides
that the client is allowed to see.

_Note: the major advantage of this is anti-cheat. Sorta hard to lift fog of war on
the client side in any meaningful way if the server is only sending stuff that falls
inside the fog of war, eh?_

Alternatively, the server can decide to create a diff, in which case it will still
create a full snapshot, but then create a CommonLogicDiff against the last snapshot
that the client was known to have seen.

In either case, the snapshot (or diff) is serialized into a proto, and sent to the
client.

### Client flow

The client receives either a snapshot or a diff, and deserializes it into the logical
object. In the case of a diff, it is applied against the base snapshot in order to
create a new Snapshot object.

The simulation time of this snapshot is compared against the current simulation time.
Snapshots for a future simulation time are set aside for application when the time
comes, and snapshots more stale than the most recent received snapshot are discarded.

The snapshot is applied to a new game state (new EnTT world registry), and client
ticks are run against the new game state until the time stamp of the prior state
matches the current simulation time.

The states of both the current client simulation and the fast-forwarded snapshot
state are used to generate a Snapshot, and a diff is created from the client
snapshot to the server message snapshot.

The client is then in charge of applying that diff as it sees fit.
* Deleted entities should generally just be deleted without delay.
* Components that do not need to be continuous in runtime can be updated in place
  * For example: nav waypoints should be set immediately
* Components that should be continuous in runtime might need to be smeared
  * For example: large position differences should be smeared across many frames
    to prevent characters from constantly jumping all over the place.
