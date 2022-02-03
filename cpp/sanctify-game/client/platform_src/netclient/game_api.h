#ifndef SANCTIFY_GAME_CLIENT_PLATFORM_SRC_NETCLIENT_GAME_API_H
#define SANCTIFY_GAME_CLIENT_PLATFORM_SRC_NETCLIENT_GAME_API_H

// Helper for making HTTP requests to the game client. Has a native and web
// implementation, both which conform to the same interface.

#include <igasync/promise.h>
#include <igcore/either.h>
#include <sanctify-game/proto/sanctify-api.pb.h>

namespace sanctify {

struct HttpResponse {
  int code;
  std::string payload;
};

class GameApi {
 public:
  GameApi(std::string url_base,
          std::shared_ptr<indigo::core::TaskList> task_list);

  std::shared_ptr<indigo::core::Promise<
      indigo::core::Either<pb::GetGameServerResponse, pb::GetGameServerError>>>
  get_game_server() const;

 protected:
  std::shared_ptr<indigo::core::Promise<HttpResponse>> http_get(
      std::string url) const;

  std::string url_base_;
  std::shared_ptr<indigo::core::TaskList> task_list_;
};

}  // namespace sanctify

#endif
