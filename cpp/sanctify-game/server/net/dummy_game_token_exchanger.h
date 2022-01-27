#ifndef SANCTIFY_GAME_SERVER_NET_DUMMY_GAME_TOKEN_EXCHANGER_H
#define SANCTIFY_GAME_SERVER_NET_DUMMY_GAME_TOKEN_EXCHANGER_H

#include <net/igametokenexchanger.h>

namespace sanctify {

/**
 * Dummy player token exchanger - basically takes a hash of the input and uses
 * that as the player ID.
 */
class DummyGameTokenExchanger : public IGameTokenExchanger {
 public:
  std::shared_ptr<GameTokenExchangePromise> exchange(
      const std::string& game_token) override;
};

}  // namespace sanctify

#endif
