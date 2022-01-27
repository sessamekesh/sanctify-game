#ifndef SANCTIFY_GAME_SERVER_APP_GAME_SERVER_H
#define SANCTIFY_GAME_SERVER_APP_GAME_SERVER_H

#include <app/input_events.h>
#include <net/igameserver.h>
#include <net/net_server.h>

#include <map>
#include <mutex>

namespace sanctify {

class GameServer : public IGameServer,
                   public std::enable_shared_from_this<GameServer> {
 public:
  static std::shared_ptr<GameServer> Create(
      GameId game_id, std::shared_ptr<NetServer> net_server);

  // TODO (sessamekesh): Start up synchronized game loop here on its own thread
  // Create a task list for game events, and allow the main application to pick
  // it up

  // IGameServer
  GameId get_game_id() const override;
  std::shared_ptr<indigo::core::Promise<bool>> add_player_to_game(
      PlayerId player_id) override;

 private:
  GameServer(GameId game_id, std::shared_ptr<NetServer> net_server);

  GameId game_id_;
  std::shared_ptr<NetServer> net_server_;

  // TODO (sessamekesh): replace this with a non-blocking queue
  // See https://github.com/max0x7ba/atomic_queue
  std::mutex pending_events_lock_;
  std::vector<GameServerEvent> pending_events_;

  std::map<PlayerId, bool> connected_players_;
};

}  // namespace sanctify

#endif
