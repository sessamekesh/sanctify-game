#include <game_scene/factory_util/build_net_client.h>
#include <netclient/game_api.h>
#include <netclient/ws_client_base.h>

using namespace sanctify;
using namespace indigo;
using namespace core;

namespace {
const char* kLogLabel = "BuildNetClient";

std::shared_ptr<Promise<Maybe<std::shared_ptr<NetClient>>>> build_net_client(
    const pb::GetGameServerResponse& game_server_data,
    std::shared_ptr<TaskList> async_task_list) {
  if (!game_server_data.has_game_server()) {
    Logger::err(kLogLabel)
        << "Weirdly malformed response - no game server found";
    return Promise<Maybe<std::shared_ptr<NetClient>>>::immediate(empty_maybe{});
  }

  // TODO (sessamekesh): get the healthy time from somewhere else
  auto net_client = std::make_shared<NetClient>(async_task_list, 30.f);

  // TODO (sessamekesh): use_wss should be a compile-time configured thing
  // (false for native - this is only for debugging)
  const bool use_wss = false;
  return net_client
      ->connect(game_server_data.game_server().hostname(),
                game_server_data.game_server().ws_port(), use_wss,
                game_server_data.game_server().game_token())
      ->then<Maybe<std::shared_ptr<NetClient>>>(
          [net_client](
              const bool& success) -> Maybe<std::shared_ptr<NetClient>> {
            if (!success) {
              Logger::err(kLogLabel) << "Failed to connect NetClient, passing "
                                        "failure back to game!";
              return empty_maybe{};
            }

            return net_client;
          },
          async_task_list);
}

}  // namespace

std::shared_ptr<Promise<Maybe<std::shared_ptr<NetClient>>>>
sanctify::build_net_client(
    std::string api_url_base,
    std::shared_ptr<indigo::core::TaskList> async_task_list) {
  auto rsl = Promise<Maybe<std::shared_ptr<NetClient>>>::create();

  GameApi api(api_url_base, async_task_list);

  api.get_game_server()->on_success(
      [rsl, async_task_list](
          const Either<pb::GetGameServerResponse, pb::GetGameServerError>&
              get_game_server_result) {
        if (get_game_server_result.is_right()) {
          Logger::err(kLogLabel)
              << "Failed to get game server from API: "
              << get_game_server_result.get_right().error_message();
          rsl->resolve(empty_maybe());
          return;
        }

        const pb::GetGameServerResponse& game_server_response =
            get_game_server_result.get_left();

        ::build_net_client(game_server_response, async_task_list)
            ->on_success(
                [rsl](const Maybe<std::shared_ptr<NetClient>>& net_client_rsl) {
                  rsl->resolve(net_client_rsl);
                },
                async_task_list);
      },
      async_task_list);

  return rsl;
}