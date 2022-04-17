#include "locomotion_system.h"

#include <common/logic/update_common/tick_time_elapsed.h>

using namespace sanctify;
using namespace logic;
using namespace indigo;
using namespace igecs;

namespace {
WorldView::Decl build_locomotion_system_decl() {
  WorldView::Decl d;
  return d.ctx_reads<CtxFrameTimeElapsed>()
      .reads<StandardNavigationParamsComponent>()
      .writes<OrientationComponent>()
      .writes<MapLocationComponent>()
      .writes<NavWaypointListComponent>();
}
const WorldView::Decl kLocomotionSystemDecl = ::build_locomotion_system_decl();
}  // namespace

const WorldView::Decl& LocomotionSystem::decl() {
  return ::kLocomotionSystemDecl;
}

void LocomotionSystem::update(WorldView* wv) {
  float dt = FrameTimeElapsedUtil::dt(wv);

  auto view =
      wv->view<MapLocationComponent, NavWaypointListComponent,
               OrientationComponent, const StandardNavigationParamsComponent>();

  for (auto [e, map_location, nav_waypoints, orientation, standard_nav_params] :
       view.each()) {
    float remaining_distance = dt * standard_nav_params.movementSpeed;

    while (remaining_distance > 0.f) {
      if (nav_waypoints.targets.size() == 0u) {
        break;
      }

      const glm::vec2& next_target = nav_waypoints.targets[0];
      glm::vec2 direction = next_target - map_location.position;
      float length = glm::length(direction);
      glm::vec2 normal = direction / length;

      if (length > remaining_distance) {
        map_location.position += normal * remaining_distance;
        break;
      }

      map_location.position = nav_waypoints.targets[0];
      nav_waypoints.targets.delete_at(0, true);
      remaining_distance -= length;
    }

    if (nav_waypoints.targets.size() == 0u) {
      wv->remove<NavWaypointListComponent>(e);
    } else {
      orientation.orientation =
          glm::atan(nav_waypoints.targets[0].x - map_location.position.x,
                    nav_waypoints.targets[0].y - map_location.position.y);
    }
  }
}
