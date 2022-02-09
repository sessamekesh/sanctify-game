#ifndef SANCTIFY_GAME_CLIENT_SRC_GAME_SCENE_NET_RECONCILE_NET_STATE_SYSTEM_H
#define SANCTIFY_GAME_CLIENT_SRC_GAME_SCENE_NET_RECONCILE_NET_STATE_SYSTEM_H

#include <igcore/bimap.h>
#include <igcore/vector.h>
#include <sanctify-game-common/gameplay/net_sync_components.h>
#include <sanctify-game-common/net/entt_snapshot_translator.h>
#include <sanctify-game-common/net/game_snapshot.h>

#include <entt/entt.hpp>
#include <set>

namespace sanctify {

class ReconcileNetStateSystem {
 public:
  ReconcileNetStateSystem();

  void advance_time_to_and_maybe_reconcile(
      entt::registry& client_world,
      std::function<void(entt::registry& server_sim, float dt)>
          update_client_sim_cb,
      float client_sim_time);
  void register_server_snapshot(const GameSnapshot& server_snapshot,
                                float client_sim_time);

  void reconcile_client_state(
      entt::registry& client_world, float client_sim_time,
      std::function<void(entt::registry& server_sim, float dt)>
          update_client_sim_cb,
      const GameSnapshot& server_snapshot);

 private:
  entt::registry server_state_;

 public:
  struct TimeSnapshot {
    GameSnapshot snapshot;

    bool operator<(const TimeSnapshot& o) const;
    bool operator==(const TimeSnapshot& o) const;
  };

  std::set<TimeSnapshot> snapshots_;

  EnttSnapshotTranslator snapshot_translator_;
};

}  // namespace sanctify

#endif
