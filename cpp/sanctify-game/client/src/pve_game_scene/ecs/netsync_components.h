#ifndef SANCTIFY_GAME_CLIENT_SRC_PVE_GAME_SCENE_ECS_NETSYNC_COMPONENTS_H
#define SANCTIFY_GAME_CLIENT_SRC_PVE_GAME_SCENE_ECS_NETSYNC_COMPONENTS_H

#include <igcore/maybe.h>
#include <igcore/vector.h>
#include <net/reconcile_net_state_system.h>
#include <net/snapshot_cache.h>
#include <sanctify-game-common/proto/sanctify-net.pb.h>

namespace sanctify {

struct SimClockComponent {
  float simClock;
};

struct OutgoingClientMessagesComponent {
  indigo::core::Vector<pb::GameClientSingleMessage> messages;
};

struct SnapshotCacheComponent {
  SnapshotCacheComponent(uint32_t max_snapshot_size)
      : cache(max_snapshot_size) {}

  SnapshotCache cache;
  ReconcileNetStateSystem reconcileNetStateSystem;
};

}  // namespace sanctify

#endif
