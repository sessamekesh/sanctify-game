#include <app/pve_game_server/ecs/netstate_components.h>
#include <app/pve_game_server/ecs/player_context_components.h>
#include <app/pve_game_server/ecs/send_client_messages_system.h>

using namespace sanctify;

namespace {
const char* kLogLabel = "player_context_components.cc";
}  // namespace

void ecs::bootstrap_server_player(
    entt::registry& world, entt::entity e,
    const pb::GameServerPlayerDescription server_desc) {
  // Attach the following components:
  // * PlayerSystemAttributes
  // * PlayerConnectionState
  // * PlayerIncomingMessageQueue
  // * PlayerOutgoingMessageQueue
  // * PlayerNetStateComponent
  world.emplace<ecs::PlayerSystemAttributes>(
      e, PlayerId{server_desc.player_id()}, server_desc.display_name(),
      server_desc.tagline());
  world.emplace<ecs::PlayerConnectionState>(
      e, PlayerConnectionState::NetState::Disconnected, false);
  world.emplace<ecs::net::PlayerOutgoingMessageQueue>(e);
  world.emplace<ecs::PlayerNetStateComponent>(e, 0.f, 0.f, 0.f, 0u);
}

const bool& ecs::is_ready(entt::registry& world, entt::entity e) {
  return world.get<ecs::PlayerConnectionState>(e).isReady;
}
