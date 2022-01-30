#include <net/build_game_token_exchanger.h>

using namespace sanctify;

std::shared_ptr<IGameTokenExchanger> sanctify::build_game_token_exchanger(TokenExchangerType type) {
  switch (type) {
    case TokenExchangerType::Dummy:
      return std::make_shared<DummyGameTokenExchanger>();
  }

  return nullptr;
}