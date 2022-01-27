#include <igcore/log.h>
#include <net/dummy_game_token_exchanger.h>
#include <net/net_server.h>

#include <CLI/CLI.hpp>
#include <chrono>
#include <iostream>
#include <map>

using namespace indigo;
using namespace core;
using namespace sanctify;
using namespace std::chrono_literals;

enum class TokenExchangerType {
  Dummy,
};

const std::map<std::string, TokenExchangerType> kTokenExchangerMap{
    {"dummy", TokenExchangerType::Dummy}};

int main(int argc, const char** argv) {
  //
  // Parse CLI params
  //
  CLI::App app{"Sanctify game server"};

  TokenExchangerType token_exchanger_type = TokenExchangerType::Dummy;

  app.add_option("-g,--game_token_exchanger", token_exchanger_type,
                 "Game token exchanger type")
      ->transform(
          CLI::CheckedTransformer(kTokenExchangerMap, CLI::ignore_case));

  CLI11_PARSE(app, argc, argv);

  //
  // Assemble server components
  //
  std::shared_ptr<IGameTokenExchanger> token_exchanger;
  switch (token_exchanger_type) {
    case TokenExchangerType::Dummy:
      token_exchanger = std::make_shared<DummyGameTokenExchanger>();
      break;
  }

  std::shared_ptr<TaskList> message_parse_task_list =
      std::make_shared<TaskList>();
  std::shared_ptr<TaskList> bookkeeping_task_list =
      std::make_shared<TaskList>();
  std::shared_ptr<EventScheduler> event_scheduler =
      std::make_shared<EventScheduler>();
  // TODO (sessamekesh): Move this outside of main, to a GameServerApp class
  // TODO (sessamekesh): Inject in server parameters instead of hard-coding them
  NetServer::Config config;
  config.Port = 9000;
  config.MessageParseTaskList = message_parse_task_list;
  config.GameTokenExchanger = token_exchanger;
  config.EventScheduler_ = event_scheduler;
  config.BookkeepingTaskList = bookkeeping_task_list;
  auto create_net_server_rsl = NetServer::Create(config);

  if (create_net_server_rsl.is_right()) {
    auto log = Logger::err("SanctifyGameServer [[main]]");
    log << "Failed to create net server:\n";
    for (int i = 0; i < create_net_server_rsl.get_right().size(); i++) {
      log << "-- " << to_string(create_net_server_rsl.get_right()[i]) << "\n";
    }
    return -1;
  }

  std::shared_ptr<NetServer> net_server = create_net_server_rsl.get_left();

  while (true) {
    bool did_task = false;
    do {
      did_task = message_parse_task_list->execute_next();
      did_task = bookkeeping_task_list->execute_next() || did_task;
    } while (did_task);

    std::this_thread::sleep_for(250ms);
  }

  std::cout << "Shutting down game server process..." << std::endl;
  return 0;
}