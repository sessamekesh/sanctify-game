#ifndef SANCTIFY_GAME_COMMON_INCLUDE_SANCTIFY_GAME_COMMON_GAMEPLAY_LOCOMOTION_H
#define SANCTIFY_GAME_COMMON_INCLUDE_SANCTIFY_GAME_COMMON_GAMEPLAY_LOCOMOTION_H

#include <igcore/maybe.h>

#include <entt/entt.hpp>
#include <glm/glm.hpp>

namespace sanctify::system {

class LocomotionSystem {
 public:
  void attach_basic_locomotion_components(entt::registry& world,
                                          entt::entity entity,
                                          glm::vec2 map_position,
                                          float movement_speed);

  void apply_standard_locomotion(entt::registry& world, float dt);

  indigo::core::Maybe<glm::vec2> get_map_position(entt::registry& world,
                                                  entt::entity entity);
};

}  // namespace sanctify::system

#endif
