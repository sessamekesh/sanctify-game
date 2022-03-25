#include <app/pve_game_server/ecs/context_components.h>
#include <app/pve_game_server/ecs/player_context_components.h>
#include <igcore/log.h>

using namespace sanctify;
using namespace indigo;
using namespace core;

namespace {
const char* kLogLabel = "context_components.cc";

ecs::GClock& get_clock(entt::registry& world) {
  return world.ctx_or_set<ecs::GClock>(0.f, 0.f);
}

ecs::GExpectedPlayers& get_expected_players(entt::registry& world) {
  return world.ctx_or_set<ecs::GExpectedPlayers>();
}

}  // namespace

const float& ecs::sim_time(entt::registry& world) {
  return ::get_clock(world).simTime;
}

const float& ecs::frame_time(entt::registry& world) {
  return ::get_clock(world).frameTime;
}

void ecs::tick(entt::registry& world, float dt) {
  auto& clock = ::get_clock(world);
  clock.frameTime = dt;
  clock.simTime += dt;
}

void ecs::add_expected_player(
    entt::registry& world, const pb::GameServerPlayerDescription& player_desc) {
  auto& expected_players = ::get_expected_players(world);

  for (int i = 0; i < expected_players.playerDescriptions.size(); i++) {
    if (expected_players.playerDescriptions[i].player_id() ==
        player_desc.player_id()) {
      // Player has already been added, this is a duplciate
      Logger::err(kLogLabel)
          << "Player already added to server :: " << player_desc.player_id()
          << " : " << player_desc.display_name();
      assert(false);
    }
  }

  entt::entity player_entity = world.create();
  ecs::bootstrap_server_player(world, player_entity, player_desc);

  expected_players.playerDescriptions.push_back(player_desc);
  expected_players.entityMap.insert(
      {PlayerId{player_desc.player_id()}, player_entity});
}

Maybe<entt::entity> ecs::get_player_entity(entt::registry& world,
                                           const PlayerId& player_id) {
  auto& expected_players = ::get_expected_players(world);

  auto it = expected_players.entityMap.find(player_id);
  if (it == expected_players.entityMap.end()) {
    Logger::err(kLogLabel) << "Failed to find entity for player "
                           << player_id.Id;
    return empty_maybe{};
  }

  return it->second;
}

void ecs::bootstrap_game_engine_callbacks(
    entt::registry& world,
    std::function<void(PlayerId, pb::GameServerMessage)> send_net_message_cb) {
  world.set<ecs::GEngineCallbacks>(send_net_message_cb);
}

void ecs::bootstrap_server_config(entt::registry& world,
                                  uint32_t max_actions_per_message,
                                  uint32_t max_queued_client_messages,
                                  uint32_t max_queued_server_actions) {
  world.set<ecs::GServerConfig>(max_actions_per_message,
                                max_queued_client_messages,
                                max_queued_server_actions);
}
