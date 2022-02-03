#include <google/protobuf/util/json_util.h>
#include <netclient/game_api.h>

using namespace sanctify;
using namespace indigo;
using namespace core;

namespace {
const char* kLogLabel = "GameApi";
}

GameApi::GameApi(std::string url_base,
                 std::shared_ptr<indigo::core::TaskList> task_list)
    : url_base_(url_base), task_list_(task_list) {}

std::shared_ptr<
    Promise<Either<pb::GetGameServerResponse, pb::GetGameServerError>>>
GameApi::get_game_server() const {
  return http_get(url_base_ + "/api/gameserver")
      ->then<Either<pb::GetGameServerResponse, pb::GetGameServerError>>(
          [](const HttpResponse& response)
              -> Either<pb::GetGameServerResponse, pb::GetGameServerError> {
            switch (response.code) {
              case 200: {
                pb::GetGameServerResponse parsed_response;
                auto rsl = google::protobuf::util::JsonStringToMessage(
                    response.payload, &parsed_response);

                if (!rsl.ok()) {
                  pb::GetGameServerError err;
                  Logger::log(kLogLabel)
                      << "Failed to parse response message: " << rsl.message();
                  err.set_error_message(
                      "Failed to parse response message, failing request");
                  return right(std::move(err));
                }

                return left(std::move(parsed_response));
              }
              default: {
                pb::GetGameServerError err;
                err.set_error_message("Unexpected return code " +
                                      std::to_string(response.code) +
                                      " from server");
                return right(std::move(err));
              }
            }
          },
          task_list_);
}