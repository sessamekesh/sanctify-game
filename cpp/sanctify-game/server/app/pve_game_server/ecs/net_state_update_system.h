#ifndef SANCTIFY_GAME_SERVER_APP_PVE_GAME_SERVER_ECS_NET_STATE_UPDATE_SYSTEM_H
#define SANCTIFY_GAME_SERVER_APP_PVE_GAME_SERVER_ECS_NET_STATE_UPDATE_SYSTEM_H

/**
 * Net state update system
 *
 * At the top of the frame, handles updates to the connection state of each
 * player (time between snapshots, reconnection to re-send messages, etc)
 */

#include <app/pve_game_server/net_event_organizer.h>

#include <entt/entt.hpp>

namespace sanctify::ecs {

class NetStateUpdateSystem {
 public:
  void update(entt::registry& world, NetEventOrganizer& net_event_organizer);
};

}  // namespace sanctify::system

#endif
