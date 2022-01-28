#ifndef SANCTIFY_GAME_SERVER_NET_I_GAME_TOKEN_EXHANGER_H
#define SANCTIFY_GAME_SERVER_NET_I_GAME_TOKEN_EXHANGER_H

#include <igasync/promise.h>
#include <igcore/either.h>
#include <util/types.h>

namespace sanctify {

enum class GameTokenExchangerError {
  InvalidGameToken,
};

struct GameTokenExchangerResponse {
  PlayerId playerId;
  GameId gameId;
};

typedef indigo::core::Promise<
    indigo::core::Either<GameTokenExchangerResponse, GameTokenExchangerError>>
    GameTokenExchangePromise;

/**
 * Game token exchanger interface. Use an implementation of this interface
 * to exchange a game token for a universally unique player ID (which
 * uniquely identifies the player)
 */
class IGameTokenExchanger {
 public:
  virtual std::shared_ptr<GameTokenExchangePromise> exchange(
      const std::string& game_token) = 0;
};

std::string to_string(const GameTokenExchangerError& err) {
  switch (err) {
    case GameTokenExchangerError::InvalidGameToken:
      return "InvalidGameToken";
  }
  return "<<GameTokenExchangerError -- Unknown>>";
}

}  // namespace sanctify

#endif
