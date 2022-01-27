#ifndef SANCTIFY_GAME_SERVER_APP_NET_IGAMESERVER_H
#define SANCTIFY_GAME_SERVER_APP_NET_IGAMESERVER_H

#include <igasync/promise.h>
#include <net/types.h>

namespace sanctify {
class IGameServer {
 public:
  virtual GameId get_game_id() const = 0;
  virtual std::shared_ptr<indigo::core::Promise<bool>> add_player_to_game(
      PlayerId player_id) = 0;
};
}  // namespace sanctify

#endif
