#ifndef SANCTIFY_GAME_SERVER_APP_SYSTEMS_LOCOMOTION_H
#define SANCTIFY_GAME_SERVER_APP_SYSTEMS_LOCOMOTION_H

#include <sanctify-game-common/gameplay/locomotion_components.h>
#include <sanctify-game-common/proto/sanctify-net.pb.h>
#include <util/types.h>

#include <entt/entt.hpp>

namespace sanctify::system {

class ServerLocomotionSystem {
 public:
  void handle_player_movement_event(const pb::PlayerMovement& action,
                                    entt::entity player_entity,
                                    entt::registry& world);
};

}  // namespace sanctify::system

#endif
