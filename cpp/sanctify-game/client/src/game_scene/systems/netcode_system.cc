#include <game_scene/systems/netcode_system.h>
#include <sanctify-game-common/gameplay/locomotion_components.h>
#include <sanctify-game-common/gameplay/net_sync_components.h>
#include <sanctify-game-common/gameplay/standard_target_travel_system.h>

using namespace sanctify;
using namespace system;
using namespace indigo;
using namespace core;

namespace {
const char* kLogLabel = "NetcodeSystem";
}

// TODO (sessamekesh): Account for clock times here!
void NetcodeSystem::update_locomotion_data(
    entt::registry& registry, const pb::PlayerStateUpdate& player_state,
    float message_server_clock_time, float expected_server_clock_time) {
  uint32_t net_id = player_state.player_id();

  entt::entity player_entity{};
  auto entity_it = net_identifier_to_entity_.find_l(net_id);
  if (entity_it == net_identifier_to_entity_.end()) {
    auto maybe_entity = try_spawn_player(registry, player_state);
    if (maybe_entity.is_empty()) {
      Logger::log(kLogLabel)
          << "Could not spawn player with netcode " << net_id;
      return;
    }
    player_entity = maybe_entity.get();
  } else {
    player_entity = *entity_it;
  }

  //
  // TODO (sessamekesh): replace the bottom segment with a time-sensitive
  //  alternative. The position received is possibly one that applied in the
  //  past, or one that possibly applies in the future - in both cases, predict
  //  what the current state should be now (incl. diff) and what it should be in
  //  the future (no diff), and set navigation params based on that.
  // This below solution will be J A N K Y
  //
  for (const auto& single_state : player_state.player_states()) {
    if (single_state.has_current_position()) {
      const auto& pb_pos = single_state.current_position();

      registry.emplace_or_replace<component::MapLocation>(
          player_entity, glm::vec2{pb_pos.x(), pb_pos.y()});
    }

    // TODO (sessamekesh): handle locomotion state here as well!
  }
}

Maybe<entt::entity> NetcodeSystem::try_spawn_player(
    entt::registry& registry, const pb::PlayerStateUpdate& player_state) {
  uint32_t net_id = player_state.player_id();

  if (net_id == 0u) {
    return empty_maybe{};
  }

  entt::entity new_player_entity = registry.create();
  registry.emplace<component::NetSyncId>(new_player_entity, net_id);
  net_identifier_to_entity_.insert_or_update(net_id, new_player_entity);

  return new_player_entity;
}