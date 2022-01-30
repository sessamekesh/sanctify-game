#ifndef SANCTIFY_GAME_SERVER_APP_GAME_SERVER_H
#define SANCTIFY_GAME_SERVER_APP_GAME_SERVER_H

#include <igasync/promise.h>
#include <sanctify-game-common/proto/sanctify-net.pb.h>
#include <util/types.h>

#include <map>
#include <mutex>

namespace sanctify {

/**
 * Game app implementation - maintains actual game state and runs game logic.
 *
 * Exposes a "Player" interface
 */
class GameServer : public std::enable_shared_from_this<GameServer> {
 public:
  using PlayerMessageCallback =
      std::function<bool(PlayerId, sanctify::pb::GameServerMessage)>;

 public:
  GameServer();
  ~GameServer();

  // Outside interface
  void receive_message_for_player(PlayerId player_id,
                                  sanctify::pb::GameClientMessage client_msg);
  void set_player_message_receiver(PlayerMessageCallback cb);
  std::shared_ptr<indigo::core::Promise<indigo::core::EmptyPromiseRsl>>
  shutdown();

 private:
  PlayerMessageCallback player_message_cb_;
};

}  // namespace sanctify

#endif
