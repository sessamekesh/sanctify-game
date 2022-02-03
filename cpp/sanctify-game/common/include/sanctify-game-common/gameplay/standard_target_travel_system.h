#ifndef SANCTIVY_GAME_COMMON_INCLUDE_SANCTIFY_GAME_COMMON_GAMEPLAY_STANDARD_TARGET_TRAVEL_SYSTEM_H
#define SANCTIVY_GAME_COMMON_INCLUDE_SANCTIFY_GAME_COMMON_GAMEPLAY_STANDARD_TARGET_TRAVEL_SYSTEM_H

#include <entt/entt.hpp>

namespace sanctify::system {

// Standard target travel system - handles standard character navigation that
// avoids terrain and other game entities, and consults the navigation mesh for
// a way to reach a destination requested by the player.
class StandardTargetTravelSystem {
 public:
  void update(entt::registry& registry, float dt);
};

}  // namespace sanctify::system

#endif
