#ifndef SANCTIFY_GAME_COMMON_INCLUDE_SANCTIFY_GAME_COMMON_GAMEPLAY_LOCOMOTION_COMPEONTNTS_H
#define SANCTIFY_GAME_COMMON_INCLUDE_SANCTIFY_GAME_COMMON_GAMEPLAY_LOCOMOTION_COMPEONTNTS_H

#include <igcore/pod_vector.h>
#include <sanctify-game-common/proto/sanctify-net.pb.h>

#include <glm/glm.hpp>

namespace sanctify::component {

struct MapLocation {
  glm::vec2 XZ;

  bool operator==(const MapLocation& o) const;
};

struct NavWaypointList {
  indigo::core::PodVector<glm::vec2> Targets;

  bool operator==(const NavWaypointList& o) const;
};

struct StandardNavigationParams {
  float MovementSpeed;

  bool operator==(const StandardNavigationParams& o) const;
};

}  // namespace sanctify::component

#endif
