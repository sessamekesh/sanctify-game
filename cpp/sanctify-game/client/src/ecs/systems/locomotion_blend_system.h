#ifndef SANCTIFY_GAME_CLIENT_SRC_ECS_SYSTEMS_LOCOMOTION_BLEND_SYSTEM_H
#define SANCTIFY_GAME_CLIENT_SRC_ECS_SYSTEMS_LOCOMOTION_BLEND_SYSTEM_H

#include <entt/entt.hpp>
#include <glm/glm.hpp>

namespace sanctify::ecs {

struct PositionSmearNetStateComponent {
  PositionSmearNetStateComponent(glm::vec2 smear_diff, float min_speed,
                                 float min_close_rate);

  /**
   * Difference between the logical map position used under the hood for
   *  updating the simulation and the position used for rendering, as a
   *  2D vector - e.g., a value of {1, 0} means that a position at {2, 3}
   *  should actually be rendered at {3, 3}.
   *
   * Expressed as a normal vector, with diffMagnitude holding the actual
   *  diff size - this is to prevent extra normalization steps during resolution
   */
  glm::vec2 mapPosDiffDirection;

  /** Magnitude of the map position diff, in world units */
  float diffMagnitude;

  /**
   * What is the minimum distance that should be covered (in world units)
   *  per second? Smears are performend no slower than this.
   *
   * This ensures that trivially small gaps are cleared instantly, and
   *  small gaps are cleared very quickly.
   */
  float minSmearSpeed;

  /**
   * What is the minimum percentage of the current gap that should be cleared
   *  per second? For example, a value of 50% on a gap of 80 world units should
   *  be cleared no slower than 40 units per second, even if minSmearSpeed is
   *  lower than 40.
   *
   * This ensures that extremely large gaps due to large disconnection time are
   *  cleared quickly.
   */
  float minCloseRate;
};

class PositionSmearNetUpdateSystem {
 public:
  void update(entt::registry& world, float dt);
};

}  // namespace sanctify::ecs

#endif
