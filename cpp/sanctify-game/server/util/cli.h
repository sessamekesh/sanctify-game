#ifndef SANCTIFY_GAME_SERVER_UTIL_CLI_H
#define SANCTIFY_GAME_SERVER_UTIL_CLI_H

#include <map>

namespace sanctify {

enum class TokenExchangerType {
  Dummy,
};

const std::map<std::string, TokenExchangerType> kTokenExchangerMap{
    {"dummy", TokenExchangerType::Dummy}};

}  // namespace sanctify

#endif
