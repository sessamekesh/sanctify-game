#ifndef SANCTIFY_GAME_SERVER_NET_BUILD_GAME_TOKEN_EXCHANGER_H
#define SANCTIFY_GAME_SERVER_NET_BUILD_GAME_TOKEN_EXCHANGER_H

#include <net/dummy_game_token_exchanger.h>
#include <util/cli.h>

namespace sanctify {

std::shared_ptr<IGameTokenExchanger> build_game_token_exchanger(
    TokenExchangerType token_exchanger_type);

// TODO
}  // namespace sanctify

#endif
