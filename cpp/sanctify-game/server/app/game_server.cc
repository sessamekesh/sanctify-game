#include <app/game_server.h>

using namespace indigo;
using namespace core;
using namespace sanctify;

std::shared_ptr<GameServer> GameServer::Create(
    GameId game_id, std::shared_ptr<NetServer> net_server) {
  return std::shared_ptr<GameServer>(new GameServer(game_id, net_server));
}

GameServer::GameServer(GameId game_id, std::shared_ptr<NetServer> net_server)
    : game_id_(game_id), net_server_(net_server) {}

GameId GameServer::get_game_id() const { return game_id_; }

std::shared_ptr<Promise<bool>> GameServer::add_player_to_game(
    PlayerId player_id) {
  auto rsl_promise = Promise<bool>::create();

  std::lock_guard l(pending_events_lock_);
  pending_events_.push_back(GameServerEvent{
      AddPlayerToGameEvent{player_id},
      [rsl_promise](bool is_success) { rsl_promise->resolve(is_success); }});
}