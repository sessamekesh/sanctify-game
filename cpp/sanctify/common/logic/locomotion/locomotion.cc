#include "locomotion.h"

using namespace sanctify;
using namespace logic;

bool MapLocationComponent::operator==(const MapLocationComponent& o) const {
  return position == o.position;
}

bool OrientationComponent::operator==(const OrientationComponent& o) const {
  return orientation == o.orientation;
}

bool NavWaypointListComponent::operator==(
    const NavWaypointListComponent& o) const {
  if (targets.size() != o.targets.size()) {
    return false;
  }

  for (int i = 0; i < targets.size(); i++) {
    if (targets[i] != o.targets[i]) {
      return false;
    }
  }

  return true;
}

bool StandardNavigationParamsComponent::operator==(
    const StandardNavigationParamsComponent& o) const {
  return movementSpeed == o.movementSpeed;
}

void LocomotionUtil::attach_locomotion_components(
    indigo::igecs::WorldView* world, entt::entity entity,
    glm::vec2 map_position, float movement_speed, float orientation) {
  world->attach<MapLocationComponent>(entity, map_position);
  world->attach<StandardNavigationParamsComponent>(entity, movement_speed);
  world->attach<OrientationComponent>(entity, orientation);
}

void LocomotionUtil::set_waypoints(indigo::igecs::WorldView* world,
                                   entt::entity e,
                                   indigo::core::PodVector<glm::vec2> targets) {
  world->remove<NavWaypointListComponent>(e);
  world->attach<NavWaypointListComponent>(e, std::move(targets));
}

glm::vec2 LocomotionUtil::get_map_position(indigo::igecs::WorldView* world,
                                           entt::entity entity) {
  return world->read<MapLocationComponent>(entity).position;
}
