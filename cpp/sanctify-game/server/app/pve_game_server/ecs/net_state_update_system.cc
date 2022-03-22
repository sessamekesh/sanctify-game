#include <app/pve_game_server/ecs/net_state_update_system.h>
#include <app/pve_game_server/ecs/player_connect_state.h>
#include <net/net_server.h>

using namespace sanctify;
using namespace system;

void NetStateUpdateSystem::update(entt::registry& world,
                                  NetEventOrganizer& net_event_organizer) {
  auto view =
      world.view<PlayerConnectStateComponent, PlayerNetStateComponent>();

  for (auto [e, player_connect_state, net_state] : view.each()) {
    const auto& pid = player_connect_state.playerId;
    auto state = net_event_organizer.net_server_state(pid);
    auto ready_state = net_event_organizer.ready_state(pid);

    switch (state) {
      case NetServer::PlayerConnectionState::ConnectedBasic:
        player_connect_state.connectionType = PlayerConnectionType::WebSocket;
        player_connect_state.connectState = PlayerConnectState::Healthy;
        break;
      case NetServer::PlayerConnectionState::ConnectedPreferred:
        player_connect_state.connectionType = PlayerConnectionType::WebRTC;
        player_connect_state.connectState = PlayerConnectState::Healthy;
        break;
      case NetServer::PlayerConnectionState::Disconnected:
      case NetServer::PlayerConnectionState::Forbidden:
      case NetServer::PlayerConnectionState::Unknown:
        player_connect_state.connectionType = PlayerConnectionType::None;
        player_connect_state.connectState = PlayerConnectState::Disconnected;
        net_state.lastSnapshotSentTime = -1.f;
        net_state.lastDiffSentTime = -1.f;
        net_state.lastAckedSnapshotId = 0u;
        break;
      case NetServer::PlayerConnectionState::Unhealthy:
        player_connect_state.connectState = PlayerConnectState::Unhealthy;
        break;
    }

    player_connect_state.readyState =
        ready_state ? PlayerReadyState::Ready : PlayerReadyState::NotReady;
  }
}