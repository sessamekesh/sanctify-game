#include <app/game_server.h>
#include <google/protobuf/util/json_util.h>
#include <igasync/promise_combiner.h>
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

namespace {
const char* kLogLabel = "main";
}

int main(int argc, const char** argv) {
  //
  // Parse CLI params
  //
  CLI::App app{"Sanctify game server"};

  TokenExchangerType token_exchanger_type = TokenExchangerType::Dummy;
  bool allow_json_messages = false;

  app.add_option("-g,--game_token_exchanger", token_exchanger_type,
                 "Game token exchanger type")
      ->transform(CLI::CheckedTransformer(sanctify::kTokenExchangerMap,
                                          CLI::ignore_case));
  app.add_option("--allow_json_encoding", allow_json_messages,
                 "Allow JSON encoding for protocol buffer messages");

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

  // TODO (sessamekesh): inject this
  uint32_t ws_port = 9001u;
  std::shared_ptr<NetServer> net_server =
      NetServer::Create(async_task_list, game_token_exchanger, event_scheduler,
                        35000u, ws_port, allow_json_messages);

  std::shared_ptr<GameServer> game_server = std::make_shared<GameServer>();

  //
  // Wire everything together...
  //
  net_server->set_on_message_callback(
      [game_server, allow_json_messages](const PlayerId& player,
                                         pb::GameClientMessage data) {
        game_server->receive_message_for_player(player, std::move(data));
      });
  net_server->set_player_connection_verify_fn(
      [game_server](
          const GameId& game_id,
          const PlayerId& player_id) -> std::shared_ptr<Promise<bool>> {
        return game_server->try_connect_player(player_id);
      });
  game_server->set_player_message_receiver(
      [net_server, async_task_list](PlayerId player_id,
                                    pb::GameServerMessage message) {
        net_server->send_message(player_id, std::move(message));
      });
  net_server->set_on_state_change_callback(
      [game_server](const PlayerId& player_id,
                    NetServer::PlayerConnectionState state) {
        if (state == NetServer::PlayerConnectionState::Disconnected) {
          game_server->notify_player_dropped_connection(player_id);
        }
        game_server->set_player_connection_state(player_id, state);
      });

  //
  // Startup all independent processes!
  //
  bool run_plz = true;
  auto combiner = PromiseCombiner::Create();
  auto net_server_start_key =
      combiner->add(net_server->configure_and_start(), async_task_list);
  combiner->add(game_server->start_game(), async_task_list);

  combiner->combine()->on_success(
      [&run_plz, net_server_start_key](
          const PromiseCombiner::PromiseCombinerResult& rsl) {
        auto& net_server_rsl = rsl.get(net_server_start_key);

        if (net_server_rsl.is_right()) {
          const PodVector<NetServer::ServerCreateError>& errors =
              net_server_rsl.get_right();
          auto log = Logger::err(kLogLabel);
          log << "Error creating net server:\n";
          for (int i = 0; i < errors.size(); i++) {
            log << "--" << to_string(errors[i]);
          }
          run_plz = false;
          return;
        }
      },
      async_task_list);

  //
  // Start process of executing tasks until the server is ready for shutdown!
  //
  Logger::log(kLogLabel)
      << "Synchronous initialization finished, beginning async task loop on "
         "main thread (this will continue until app shutdown)";
  while (run_plz) {
    while (async_task_list->execute_next()) {
    }

    std::this_thread::sleep_for(1ms);
  }

  Logger::log(kLogLabel)
      << "App shutdown condition fired - terminating systems...";

  net_server->shutdown();
  game_server->shutdown();

  return 0;
}