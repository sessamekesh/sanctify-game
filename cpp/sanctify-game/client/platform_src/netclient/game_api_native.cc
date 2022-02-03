#include <netclient/game_api.h>

#include <future>

#include "HTTPRequest.hpp"

using namespace sanctify;
using namespace indigo;
using namespace core;

namespace {
const char* kLogLabel = "GameApi ~Native~";
}

std::shared_ptr<Promise<HttpResponse>> GameApi::http_get(
    std::string url) const {
  auto rsl = Promise<HttpResponse>::create();

  task_list_->add_task(Task::of([rsl, url]() {
    http::Request request{url};

    try {
      const auto response =
          request.send("GET", "", {}, std::chrono::seconds(5));

      std::string body(response.body.size(), '\0');
      memcpy(&body[0], &response.body[0], response.body.size());
      rsl->resolve(HttpResponse{response.status, std::move(body)});
    } catch (const std::exception& e) {
      Logger::log(kLogLabel) << "HTTP request failed, error: " << e.what();
      rsl->resolve(HttpResponse{0, ""});
    }
  }));

  return rsl;
}