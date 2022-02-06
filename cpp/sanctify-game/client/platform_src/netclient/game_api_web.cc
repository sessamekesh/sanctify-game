#include <emscripten/fetch.h>
#include <netclient/game_api.h>

#include <map>
#include <mutex>

using namespace sanctify;
using namespace indigo;
using namespace core;

namespace {

std::mutex mut_gNextReqId;
uint32_t gNextReqId = 1u;

std::map<uint32_t, Request> pending_requests_;

struct Request {
  std::shared_ptr<Promise<HttpResponse>> responsePromise;
};

void em_fetch_success(emscripten_fetch_t* fetch) {
  if (!fetch->userData) {
    return;
  }

  int reqId = (int)fetch->userData;

  std::lock_guard<std::mutex> l(mut_gNextReqId);
  auto it = pending_requests_.find(reqId);
  if (it == pending_requests_.end()) {
    return;
  }

  auto rsl = it->second.responsePromise;

  std::string data(fetch->numBytes);
  memcpy(&data[0], fetch->data, fetch->numBytes);
  rsl->response(HttpResponse{fetch->status, std::move(data)});

  pending_requests_.erase(it);
}

void em_fetch_error(emscripten_fetch_t* fetch) {
  if (!fetch->userData) {
    return;
  }

  int reqId = (int)fetch->userData;

  std::lock_guard<std::mutex> l(mut_gNextReqId);
  auto it = pending_requests_.find(reqId);
  if (it == pending_requests_.end()) {
    return;
  }

  auto rsl = it->second.responsePromise;

  std::string data(fetch->numBytes);
  memcpy(&data[0], fetch->data, fetch->numBytes);
  rsl->response(HttpResponse{fetch->status, std::move(data)});

  pending_requests_.erase(it);
}

const char* kLogLabel = "GameApi ~Web~";

}  // namespace

std::shared_ptr<Promise<HttpResponse>> GameApi::http_get(
    std::string url) const {
  auto rsl = Promise<HttpResponse>::create();

  emscripten_fetch_attr_t attr;
  emscripten_fetch_attr_init(&attr);
  strcpy(attr.requestMethod, "GET");
  attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;

  Request r{rsl};

  std::lock_guard<std::mutex> l(mut_gNextReqId);
  auto req_id = gNextReqId++;
  pending_requests_.emplace(req_id, r);

  attr.userData = (void*)req_id;
  attr.onsuccess = ::em_fetch_success;
  attr.onerror = ::em_fetch_error;

  emscripten_fetch(&attr, url.c_str());

  return rsl;
}