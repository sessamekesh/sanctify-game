#include <igcore/log.h>
#include <util/cli.h>

#include <CLI/CLI.hpp>
#include <chrono>
#include <iostream>

using namespace indigo;
using namespace core;
using namespace sanctify;
using namespace std::chrono_literals;

int main(int argc, const char** argv) {
  //
  // Parse CLI params
  //
  CLI::App app{"Sanctify game server"};

  TokenExchangerType token_exchanger_type = TokenExchangerType::Dummy;

  app.add_option("-g,--game_token_exchanger", token_exchanger_type,
                 "Game token exchanger type")
      ->transform(CLI::CheckedTransformer(sanctify::kTokenExchangerMap,
                                          CLI::ignore_case));

  CLI11_PARSE(app, argc, argv);

  std::cerr << "Server not yet implemented" << std::endl;
  return -1;
}