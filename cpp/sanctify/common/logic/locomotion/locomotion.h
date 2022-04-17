#ifndef SANCTIFY_COMMON_LOGIC_LOCOMOTION_LOCOMOTION_H
#define SANCTIFY_COMMON_LOGIC_LOCOMOTION_LOCOMOTION_H

#include <igcore/pod_vector.h>
#include <igecs/world_view.h>

#include <entt/entt.hpp>
#include <glm/glm.hpp>

namespace sanctify::logic {

struct MapLocationComponent {
  glm::vec2 position;

  bool operator==(const MapLocationComponent& o) const;
};

struct OrientationComponent {
  float orientation;

  bool operator==(const OrientationComponent& o) const;
};

struct NavWaypointListComponent {
  indigo::core::PodVector<glm::vec2> targets;

  bool operator==(const NavWaypointListComponent& o) const;
};

struct StandardNavigationParamsComponent {
  float movementSpeed;

  bool operator==(const StandardNavigationParamsComponent& o) const;
};

class LocomotionUtil {
 public:
  static void attach_locomotion_components(indigo::igecs::WorldView* world,
                                           entt::entity entity,
                                           glm::vec2 map_position,
                                           float movement_speed,
                                           float orientation = 0.f);

  static void set_waypoints(indigo::igecs::WorldView* world, entt::entity e,
                            indigo::core::PodVector<glm::vec2> targets);

  static glm::vec2 get_map_position(indigo::igecs::WorldView* world,
                                    entt::entity entity);
};

}  // namespace sanctify::logic

#endif
