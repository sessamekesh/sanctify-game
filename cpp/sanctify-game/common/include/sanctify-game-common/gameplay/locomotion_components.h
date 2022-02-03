#ifndef SANCTIFY_GAME_COMMON_INCLUDE_SANCTIFY_GAME_COMMON_GAMEPLAY_LOCOMOTION_COMPEONTNTS_H
#define SANCTIFY_GAME_COMMON_INCLUDE_SANCTIFY_GAME_COMMON_GAMEPLAY_LOCOMOTION_COMPEONTNTS_H

#include <sanctify-game-common/proto/sanctify-net.pb.h>

#include <glm/glm.hpp>

namespace sanctify::component {

struct MapLocation {
  glm::vec2 XZ;
};

struct StandardNavigationParams {
  float MovementSpeed;
};

struct TravelToLocation {
  glm::vec2 Target;

  static void serialize(const TravelToLocation& v,
                        pb::TravelToLocationRequest* o);
  static void deserialize(const pb::TravelToLocationRequest& v,
                          TravelToLocation* o);
};

}  // namespace sanctify::component

#endif
