#ifndef SANCTIFY_GAME_SERVER_APP_NET_EVENTS_H
#define SANCTIFY_GAME_SERVER_APP_NET_EVENTS_H

#include <igasync/promise.h>
#include <util/types.h>

namespace sanctify {

struct ConnectPlayerEvent {
  PlayerId playerId;
  std::shared_ptr<indigo::core::Promise<bool>> connectionPromise;
};

struct DisconnectPlayerEvent {
  PlayerId playerId;
};

typedef std::variant<ConnectPlayerEvent, DisconnectPlayerEvent> NetEvent;

}  // namespace sanctify

#endif
