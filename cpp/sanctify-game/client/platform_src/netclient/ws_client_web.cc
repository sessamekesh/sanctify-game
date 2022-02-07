#include <igcore/log.h>
#include <netclient/ws_client_web.h>

#include <map>
#include <mutex>

using namespace sanctify;
using namespace indigo;
using namespace core;

namespace {
const char* kLogLabel = "WsClientWeb";

std::mutex mut_gCtxVal;
int gCtxVal = 2;

std::map<int, std::weak_ptr<WsClientWeb>> gWebClients;

std::shared_ptr<WsClientWeb> get_web_client(int ctx_val) {
  std::lock_guard<std::mutex> l(::mut_gCtxVal);
  auto it = ::gWebClients.find(ctx_val);
  if (it == ::gWebClients.end()) {
    return nullptr;
  }

  return it->second.lock();
}

int set_web_client(std::weak_ptr<WsClientWeb> ptr) {
  std::lock_guard<std::mutex> l(::mut_gCtxVal);
  auto client_id = ::gCtxVal++;
  ::gWebClients[client_id] = ptr;
  return client_id;
}

void disconnect_web_client(int ctx_val) {
  std::lock_guard<std::mutex> l(::mut_gCtxVal);
  ::gWebClients.erase(ctx_val);
}

EM_BOOL _on_open(int evt_type, const EmscriptenWebSocketOpenEvent* ws_event,
                 void* user_data) {
  int ctx_val = (int)user_data;
  auto web_client = ::get_web_client(ctx_val);

  if (web_client != nullptr) {
    web_client->on_open(ws_event->socket);
  }

  return true;
}

EM_BOOL _on_error(int evt_type, const EmscriptenWebSocketErrorEvent* ws_event,
                  void* user_data) {
  int ctx_val = (int)user_data;
  auto web_client = ::get_web_client(ctx_val);

  if (web_client != nullptr) {
    web_client->on_error(ws_event->socket);
  }

  return true;
}

EM_BOOL _on_close(int evt_type, const EmscriptenWebSocketCloseEvent* ws_event,
                  void* user_data) {
  int ctx_val = (int)user_data;
  auto web_client = ::get_web_client(ctx_val);

  if (web_client != nullptr) {
    web_client->on_close(ws_event->socket);
  }

  return true;
}

EM_BOOL _on_message(int evt_type,
                    const EmscriptenWebSocketMessageEvent* ws_event,
                    void* user_data) {
  int ctx_val = (int)user_data;
  auto web_client = ::get_web_client(ctx_val);

  if (web_client != nullptr) {
    std::string payload(ws_event->numBytes, '\0');
    memcpy(&payload[0], ws_event->data, ws_event->numBytes);

    web_client->on_message(ws_event->socket, payload);
  }

  return true;
}

}  // namespace

WsClientWeb::WsClientWeb(
    std::shared_ptr<indigo::core::TaskList> message_parse_task_list,
    float healthy_time)
    : WsClient(message_parse_task_list, healthy_time),
      is_connected_(false),
      ws_(0),
      global_ctx_val_(0) {}

WsClientWeb::~WsClientWeb() {
  if (is_connected_) {
    ::disconnect_web_client(global_ctx_val_);
    is_connected_ = false;
    global_ctx_val_ = 0;
  }
}

std::shared_ptr<Promise<bool>> WsClientWeb::inner_connect(std::string url) {
  auto that = shared_from_this();
  auto rsl_promise = Promise<bool>::create();

  EmscriptenWebSocketCreateAttributes wsdesc{url.c_str(), nullptr, true};
  ws_ = emscripten_websocket_new(&wsdesc);

  auto weak_that = weak_from_this();
  global_ctx_val_ = ::set_web_client(weak_that);

  emscripten_websocket_set_onopen_callback(ws_, (void*)global_ctx_val_,
                                           ::_on_open);
  emscripten_websocket_set_onerror_callback(ws_, (void*)global_ctx_val_,
                                            ::_on_error);
  emscripten_websocket_set_onclose_callback(ws_, (void*)global_ctx_val_,
                                            ::_on_close);
  emscripten_websocket_set_onmessage_callback(ws_, (void*)global_ctx_val_,
                                              ::_on_message);

  rsl_promise->resolve(true);

  return rsl_promise;
}

void WsClientWeb::on_open(EMSCRIPTEN_WEBSOCKET_T socket) {
  if (socket != ws_) {
    return;
  }

  Logger::log(kLogLabel) << "WebSocket connection opened!";
  is_connected_ = true;

  on_connect();
}

void WsClientWeb::on_close(EMSCRIPTEN_WEBSOCKET_T socket) {
  if (socket != ws_) {
    return;
  }

  on_disconnect();
}

void WsClientWeb::on_error(EMSCRIPTEN_WEBSOCKET_T socket) {
  if (socket != ws_) {
    WsClient::on_error("-- err --");
  }
}

void WsClientWeb::on_message(EMSCRIPTEN_WEBSOCKET_T socket, std::string data) {
  if (socket != ws_) {
    WsClient::on_error("-- err --");
  }

  recv_raw_msg(std::move(data));
}

void WsClientWeb::send_raw_msg(std::string raw_msg) {
  if (is_connected_) {
    emscripten_websocket_send_binary(ws_, &raw_msg[0], raw_msg.size());
  }
}

void WsClientWeb::destroy_connection() {
  if (!is_connected_) {
    return;
  }

  emscripten_websocket_close(ws_, 1000, "---closed---");

  ::disconnect_web_client(global_ctx_val_);
  is_connected_ = false;
  global_ctx_val_ = 0;
}