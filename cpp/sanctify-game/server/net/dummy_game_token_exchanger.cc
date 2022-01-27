#include <net/dummy_game_token_exchanger.h>

#include <functional>

using namespace indigo;
using namespace core;
using namespace sanctify;

std::shared_ptr<GameTokenExchangePromise> DummyGameTokenExchanger::exchange(
    const std::string& game_token) {
  uint64_t str_hash = (uint64_t)std::hash<std::string>{}(game_token);

  return GameTokenExchangePromise::immediate(
      left(GameTokenExchangerResponse{PlayerId{str_hash}, GameId{1ull}}));
}