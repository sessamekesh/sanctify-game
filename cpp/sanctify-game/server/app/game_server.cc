#include <app/game_server.h>
#include <igcore/log.h>

using namespace indigo;
using namespace core;
using namespace sanctify;

namespace {
const char* kLogLabel = "GameServer";

bool default_player_message_handler(PlayerId player_id,
                                    indigo::net::pb::GameServerMessage msg) {
  Logger::err(kLogLabel)
      << "No player message handler defined - message for player "
      << player_id.Id << " is dropped!";

  return false;
}
}  // namespace

GameServer::GameServer()
    : player_message_cb_(::default_player_message_handler) {}

GameServer::~GameServer() {}

void GameServer::receive_message_for_player(
    PlayerId player_id, indigo::net::pb::GameClientMessage client_msg) {}

void GameServer::set_player_message_receiver(PlayerMessageCallback cb) {
  player_message_cb_ = cb;
}

std::shared_ptr<indigo::core::Promise<indigo::core::EmptyPromiseRsl>>
GameServer::shutdown() {
  // Begin shutdown process, and resolve the promise when the game server has
  // completed its shutdown
  return Promise<EmptyPromiseRsl>::immediate(EmptyPromiseRsl{});
}