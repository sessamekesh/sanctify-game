#ifndef SANCTIFY_GAME_COMMON_INCLUDE_SANCTIFY_GAME_COMMON_GAMEPLAY_PLAYER_DEFINITION_COMPONENTS_H
#define SANCTIFY_GAME_COMMON_INCLUDE_SANCTIFY_GAME_COMMON_GAMEPLAY_PLAYER_DEFINITION_COMPONENTS_H

namespace sanctify::component {

struct BasicPlayerComponent {
  bool operator==(const BasicPlayerComponent& o) const { return this == &o; }
};

}  // namespace sanctify::component

#endif
