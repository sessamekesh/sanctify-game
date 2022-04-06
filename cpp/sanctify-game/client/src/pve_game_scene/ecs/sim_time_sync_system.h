#ifndef SANCTIFY_GAME_CLIENT_SRC_PVE_GAME_SCENE_ECS_SIM_TIME_SYNC_SYSTEM_H
#define SANCTIFY_GAME_CLIENT_SRC_PVE_GAME_SCENE_ECS_SIM_TIME_SYNC_SYSTEM_H

#include <igcore/pod_vector.h>
#include <pve_game_scene/ecs/netsync_components.h>

#include <entt/entt.hpp>

namespace sanctify {

class SimTimeSyncSystem {
 public:
  struct LoadTimeDeltaComponent {
    LoadTimeDeltaComponent(float load_time);

    float loadTimeLeft;
    float nextPingTime;

    uint32_t nextPingId;

    bool hasPerformedReconcile;

    indigo::core::PodVector<float> timeDeltas;
    indigo::core::PodVector<uint32_t> expectedPingIds;
  };

 public:
  void loading_update(entt::registry& world, float dt);
  void handle_pong(entt::registry& world, const pb::ServerPong& msg);
  bool is_done_loading(entt::registry& world);
};

}  // namespace sanctify

#endif
