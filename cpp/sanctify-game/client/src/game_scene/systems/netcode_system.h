#ifndef SANCTIFY_GAME_CLIENT_SRC_GAME_SCENE_SYSTEMS_NETCODE_SYSTEM_H
#define SANCTIFY_GAME_CLIENT_SRC_GAME_SCENE_SYSTEMS_NETCODE_SYSTEM_H

#include <igcore/bimap.h>
#include <igcore/maybe.h>
#include <sanctify-game-common/net/net_config.h>
#include <sanctify-game-common/proto/sanctify-net.pb.h>

#include <entt/entt.hpp>

//
// Netcode System
//
// Handles updates for individual protocol buffer messages
//

namespace sanctify::system {

class NetcodeSystem {
 public:
  void update_locomotion_data(entt::registry& registry,
                              const pb::PlayerStateUpdate& player_state,
                              float message_server_clock_time,
                              float expected_server_clock_time);

 private:
  indigo::core::Maybe<entt::entity> try_spawn_player(
      entt::registry& registry, const pb::PlayerStateUpdate& player_state);

 private:
  // Relate netcode identifiers to corresponding entt::entities (and back)
  indigo::core::Bimap<uint32_t, entt::entity> net_identifier_to_entity_;
};

}  // namespace sanctify::system

#endif
