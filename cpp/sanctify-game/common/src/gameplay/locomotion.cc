#include <sanctify-game-common/gameplay/locomotion.h>
#include <sanctify-game-common/gameplay/locomotion_components.h>

using namespace sanctify;
using namespace system;
using namespace indigo;
using namespace core;

void LocomotionSystem::attach_basic_locomotion_components(
    entt::registry& world, entt::entity entity, glm::vec2 map_position,
    float movement_speed) {
  world.emplace<component::MapLocation>(entity, map_position);
  world.emplace<component::StandardNavigationParams>(entity, movement_speed);
}

void LocomotionSystem::apply_standard_locomotion(entt::registry& world,
                                                 float dt) {
  auto view = world.view<component::MapLocation, component::NavWaypointList,
                         const component::StandardNavigationParams>();

  for (auto [entity, map_location, nav_waypoints, standard_nav_params] :
       view.each()) {
    float remaining_distance = dt * standard_nav_params.MovementSpeed;

    while (remaining_distance > 0.f) {
      if (nav_waypoints.Targets.size() == 0u) {
        break;
      }

      const glm::vec2& next_target = nav_waypoints.Targets[0];
      glm::vec2 direction = next_target - map_location.XZ;
      float length = glm::length(direction);
      glm::vec2 normal = direction / length;

      if (length > remaining_distance) {
        map_location.XZ += normal * remaining_distance;
        break;
      }

      map_location.XZ = nav_waypoints.Targets[0];
      nav_waypoints.Targets.delete_at(0, true);
      remaining_distance -= length;
    }

    if (nav_waypoints.Targets.size() == 0) {
      world.remove<component::NavWaypointList>(entity);
    }
  }
}

Maybe<glm::vec2> LocomotionSystem::get_map_position(entt::registry& world,
                                                    entt::entity entity) {
  component::MapLocation* map_location =
      world.try_get<component::MapLocation>(entity);

  if (map_location == nullptr) {
    return empty_maybe{};
  }

  return map_location->XZ;
}
