#ifndef SANCTIFY_GAME_SERVER_APP_ECS_PLAYER_NAV_SYSTEM_H
#define SANCTIFY_GAME_SERVER_APP_ECS_PLAYER_NAV_SYSTEM_H

/**
 * Component and system for responding to player navigation events for a player
 */

#include <ignav/detour_navmesh.h>

#include <entt/entt.hpp>
#include <glm/glm.hpp>

namespace sanctify {

namespace component {
struct PlayerNavRequestComponent {
  glm::vec2 requestPosition;

  static void attach_on(entt::registry& world, entt::entity e, glm::vec2 pos);
};
}  // namespace component

namespace system {
class PlayerNavSystem {
 public:
  void update(entt::registry& world, const indigo::nav::DetourNavmesh& navmesh);
};
}  // namespace system

}  // namespace sanctify

#endif
