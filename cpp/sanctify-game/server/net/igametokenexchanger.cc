#include <net/igametokenexchanger.h>

using namespace sanctify;

std::string sanctify::to_string(const GameTokenExchangerError& err) {
  switch (err) {
    case GameTokenExchangerError::InvalidGameToken:
      return "InvalidGameToken";
  }
  return "<<GameTokenExchangerError -- Unknown>>";
}