#ifndef SANCTIFY_GAME_SERVER_APP_SYSTEMS_LOCOMOTION_H
#define SANCTIFY_GAME_SERVER_APP_SYSTEMS_LOCOMOTION_H

#include <igcore/maybe.h>
#include <ignav/detour_navmesh.h>
#include <sanctify-game-common/gameplay/locomotion_components.h>
#include <sanctify-game-common/proto/sanctify-net.pb.h>
#include <util/types.h>

#include <entt/entt.hpp>
#include <map>

namespace sanctify::system {

class ServerLocomotionSystem {
 public:
  indigo::core::Maybe<glm::vec2> handle_player_movement_event(
      const pb::PlayerMovement& action, entt::entity player_entity,
      entt::registry& world, const indigo::nav::DetourNavmesh& navmesh);
};

}  // namespace sanctify::system

#endif
