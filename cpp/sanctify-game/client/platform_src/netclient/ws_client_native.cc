#include <netclient/ws_client_base.h>
#include <netclient/ws_client_native.h>

using namespace sanctify;
using namespace indigo;
using namespace core;

namespace {
const char* kLogLabel = "WsClientNative";
}

WsClientNative::WsClientNative(
    std::shared_ptr<indigo::core::TaskList> message_parse_task_list,
    float healthy_time)
    : WsClient(message_parse_task_list, healthy_time),
      ws_(empty_maybe{}),
      is_running_(false) {}

std::shared_ptr<Promise<bool>> WsClientNative::inner_connect(std::string url) {
  auto that = shared_from_this();

  auto rsl_promise = Promise<bool>::create();

  ws_ = WsInternal{};

  auto& client = client_;

  client.clear_access_channels(websocketpp::log::alevel::frame_header);
  client.clear_access_channels(websocketpp::log::alevel::frame_payload);
  client.set_error_channels(websocketpp::log::elevel::rerror);

  client.init_asio();

  client.set_open_handler(
      std::bind(WsClientNative::_on_open, that, std::placeholders::_1));
  client.set_fail_handler(
      std::bind(WsClientNative::_on_fail, that, std::placeholders::_1));
  client.set_close_handler(
      std::bind(WsClientNative::_on_close, that, std::placeholders::_1));
  client.set_message_handler(std::bind(WsClientNative::_on_message, that,
                                       std::placeholders::_1,
                                       std::placeholders::_2));

  std::error_code ec;
  ec.clear();
  auto conn = client.get_connection(url, ec);
  if (ec) {
    Logger::err(kLogLabel) << "Error getting connection for websocket: "
                           << ec.message();
    rsl_promise->resolve(false);
  }

  client.connect(conn);

  std::thread([l = that, &client]() {
    Logger::log("ws_thread") << "Starting WS thread...";
    client.run();
    Logger::log("ws_thread") << "Ending WS thread...";
  }).detach();

  return rsl_promise;
}

void WsClientNative::_on_open(std::shared_ptr<WsClientNative> client,
                              websocketpp::connection_hdl hdl) {
  client->on_open(hdl);
}

void WsClientNative::on_open(websocketpp::connection_hdl hdl) {
  if (ws_.has_value()) {
    ws_.get().connection = hdl;
  }
  on_connect();
}

void WsClientNative::_on_fail(std::shared_ptr<WsClientNative> client,
                              websocketpp::connection_hdl hdl) {
  client->on_fail(hdl);
}

void WsClientNative::on_fail(websocketpp::connection_hdl hdl) {
  on_error("Fail");
}

void WsClientNative::_on_close(std::shared_ptr<WsClientNative> client,
                               websocketpp::connection_hdl hdl) {
  client->on_close(hdl);
}

void WsClientNative::on_close(websocketpp::connection_hdl) {
  on_disconnect();
  destroy_connection();
}

void WsClientNative::_on_message(std::shared_ptr<WsClientNative> client,
                                 websocketpp::connection_hdl hdl,
                                 WsClientNative::WSMsgPtr msg_ptr) {
  client->on_message(hdl, msg_ptr);
}

void WsClientNative::on_message(websocketpp::connection_hdl,
                                WsClientNative::WSMsgPtr msg_ptr) {
  recv_raw_msg(msg_ptr->get_payload());
}

void WsClientNative::send_raw_msg(std::string raw_msg) {
  pb::GameClientMessage msg{};
  if (!msg.ParseFromString(raw_msg)) {
    Logger::err(kLogLabel) << "Somehow our own message fails?";
  }

  // This is also shitty, we have to convert to binary because this data is
  // binary...
  if (ws_.has_value()) {
    std::error_code ec;
    ec.clear();
    client_.send(ws_.get().connection, raw_msg,
                 websocketpp::frame::opcode::binary, ec);
    if (ec) {
      Logger::err(kLogLabel) << "Failed to send WS message: " << ec.message();
    }
  }
}

void WsClientNative::destroy_connection() {
  if (ws_.has_value()) {
    std::error_code ec;
    ec.clear();
    client_.close(ws_.get().connection, websocketpp::close::status::normal, "",
                  ec);
    if (ec) {
      Logger::err(kLogLabel)
          << "Failed to close WS connection: " << ec.message();
    }
  }
  ws_ = empty_maybe{};
  is_running_ = false;
}