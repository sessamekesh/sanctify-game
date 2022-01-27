#ifndef SANCTIFY_GAME_SERVER_APP_INPUT_EVENTS_H
#define SANCTIFY_GAME_SERVER_APP_INPUT_EVENTS_H

#include <net/types.h>

#include <functional>
#include <variant>

namespace sanctify {

struct AddPlayerToGameEvent {
  PlayerId playerId;
};

struct PlayerDisconnectedEvent {
  PlayerId playerId;
};

// TODO (sessamekesh): variant for all the various event types that can be sent
// in, and an object that consumes the variant.

using GameServerInputMessage =
    std::variant<AddPlayerToGameEvent, PlayerDisconnectedEvent>;

struct GameServerEvent {
  GameServerInputMessage Input;
  std::function<void(bool)> OnFinish;
};

}  // namespace sanctify

#endif
