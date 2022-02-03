#include <sanctify-game-common/gameplay/locomotion_components.h>
#include <sanctify-game-common/gameplay/standard_target_travel_system.h>

using namespace sanctify;
using namespace system;

void StandardTargetTravelSystem::update(entt::registry& registry, float dt) {
  auto view = registry.view<component::MapLocation,
                            const component::StandardNavigationParams,
                            const component::TravelToLocation>();

  // For now, this is just a basic thing - advance directly towards the target
  for (auto [entity, map_location, nav_params, dest] : view.each()) {
    glm::vec2 diff = map_location.XZ - dest.Target;
    float len = diff.length();
    glm::vec2 dir = diff / len;

    float max_distance = nav_params.MovementSpeed * dt;
    if (max_distance >= len) {
      map_location.XZ = dest.Target;
      registry.remove<component::TravelToLocation>(entity);
      continue;
    }

    map_location.XZ += dir * max_distance;
  }
}