#include <igcore/log.h>
#include <net/build_game_token_exchanger.h>
#include <net/net_server.h>
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

  //
  // Put together app resources
  //
  std::shared_ptr<sanctify::IGameTokenExchanger> game_token_exchanger =
      sanctify::build_game_token_exchanger(token_exchanger_type);
  std::shared_ptr<TaskList> async_task_list = std::make_shared<TaskList>();
  std::shared_ptr<EventScheduler> event_scheduler =
      std::make_shared<EventScheduler>();

  if (game_token_exchanger == nullptr) {
    std::cerr << "Failed to build game token exchanger" << std::endl;
    return -1;
  }

  std::shared_ptr<NetServer> net_server =
      NetServer::Create(async_task_list, game_token_exchanger, event_scheduler,
                        35000u, 9001u, true);

  net_server->set_on_message_callback(
      [](const PlayerId& player, indigo::core::RawBuffer data) -> bool {
        size_t prefix_size = std::min<size_t>(data.size(), 24);
        std::string prefix(prefix_size, '\0');
        memcpy(&prefix[0], data.get(), prefix_size);
        Logger::log("game_msg")
            << "Player: " << player.Id << ", message: " << prefix;
        return true;
      });
  net_server->set_player_connection_verify_fn(
      [](const GameId& game_id,
         const PlayerId& player_id) -> std::shared_ptr<Promise<bool>> {
        Logger::log("game_msg") << "Connecting player " << player_id.Id
                                << " to game " << game_id.Id;
        return Promise<bool>::immediate(true);
      });

  bool run_plz = true;
  net_server->configure_and_start()->on_success(
      [&run_plz](const NetServer::NetServerCreatePromiseT::RslType& rsl) {
        if (rsl.is_right()) {
          const PodVector<NetServer::ServerCreateError>& errors =
              rsl.get_right();
          auto log = Logger::err("main");
          log << "Error creating net server:\n";
          for (int i = 0; i < errors.size(); i++) {
            log << "--" << to_string(errors[i]);
          }
          run_plz = false;
          return;
        }

        // rsl.get_left(); // Ha, don't need this, eh? Already have the net
        // server!
      },
      async_task_list);

  while (run_plz) {
    while (async_task_list->execute_next()) {
    }

    std::this_thread::sleep_for(10ms);
  }

  net_server->shutdown();

  return 0;
}