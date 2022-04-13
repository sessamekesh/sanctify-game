#ifndef SANCTIFY_GAME_CLIENT_PVE_GAME_SCENE_ECS_MOVEMENT_INDICATOR_RENDER_SYSTEM_H
#define SANCTIFY_GAME_CLIENT_PVE_GAME_SCENE_ECS_MOVEMENT_INDICATOR_RENDER_SYSTEM_H

#include <entt/entt.hpp>
#include <glm/glm.hpp>

namespace sanctify::pve {

class MovementIndicatorRenderSystem {
 public:
  /** Returns true if the ECS world is ready for adding movement indicators */
  static bool is_ready(entt::registry& world);

  /** Add a new indicator at the given game position */
  static void add_indicator(entt::registry& world, glm::vec2 position);

  /** Shrink existing indicators, and remove dead ones */
  static void update(entt::registry& world, float dt);
};

}  // namespace sanctify::pve

#endif
