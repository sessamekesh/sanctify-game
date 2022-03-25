#include <app/pve_game_server/ecs/net_state_update_system.h>
#include <app/pve_game_server/ecs/player_context_components.h>
#include <app/pve_game_server/ecs/send_client_messages_system.h>
#include <net/net_server.h>

using namespace sanctify;
using namespace ecs;

void NetStateUpdateSystem::update(entt::registry& world,
                                  NetEventOrganizer& net_event_organizer) {
  auto view =
      world.view<ecs::PlayerSystemAttributes, ecs::PlayerConnectionState,
                 ecs::PlayerNetStateComponent>();

  for (auto [e, player_attribs, player_connect_state, net_state] :
       view.each()) {
    const auto& pid = player_attribs.playerId;
    auto state = net_event_organizer.net_server_state(pid);

    switch (state) {
      case NetServer::PlayerConnectionState::ConnectedBasic:
      case NetServer::PlayerConnectionState::ConnectedPreferred:
        player_connect_state.netState =
            ecs::PlayerConnectionState::NetState::Healthy;
        break;
      case NetServer::PlayerConnectionState::Disconnected:
      case NetServer::PlayerConnectionState::Forbidden:
      case NetServer::PlayerConnectionState::Unknown:
        player_connect_state.netState =
            ecs::PlayerConnectionState::NetState::Disconnected;
        net_state.lastSnapshotSentTime = -1.f;
        net_state.lastDiffSentTime = -1.f;
        net_state.lastAckedSnapshotId = 0u;
        break;
      case NetServer::PlayerConnectionState::Unhealthy:
        player_connect_state.netState =
            ecs::PlayerConnectionState::NetState::Unhealthy;
        break;
    }

    player_connect_state.isReady = net_event_organizer.ready_state(pid);
  }
}
