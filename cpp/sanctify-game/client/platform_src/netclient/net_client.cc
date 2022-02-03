#include <netclient/net_client.h>

using namespace sanctify;
using namespace indigo;
using namespace core;

namespace {
const char* kLogLabel = "NetClient";
}

NetClient::NetClient(std::shared_ptr<WsClient> ws_client_impl)
    : state_(NetClient::ConnectionState::Disconnected),
      ws_client_(ws_client_impl) {}

std::shared_ptr<Promise<bool>> NetClient::connect(std::string hostname,
                                                  uint16_t ws_port,
                                                  bool use_wss,
                                                  std::string game_token) {
  ws_client_->destroy_connection();

  auto that = shared_from_this();
  ws_client_->set_on_connection_state_changed(
      [that](WsClient::ConnectionState ws_state) {
        switch (ws_state) {
          case WsClient::ConnectionState::Unhealthy:
            that->set_state(ConnectionState::Unhealthy);
            break;
          case WsClient::ConnectionState::Healthy:
            that->set_state(ConnectionState::Basic);
            break;
          case WsClient::ConnectionState::MisconfiguredConnection:
            that->set_state(ConnectionState::Malformed);
            break;
          default:
            that->set_state(ConnectionState::Disconnected);
            break;
        }
      });
  ws_client_->set_on_server_message([that](pb::GameServerMessage msg) {
    if (that->on_server_msg_cb_) {
      that->on_server_msg_cb_(std::move(msg));
    }
  });

  return ws_client_->connect(hostname, ws_port, use_wss, game_token);
}

void NetClient::tick_clock(float dt) { ws_client_->tick_clock(dt); }

void NetClient::set_connection_state_changed_listener(
    std::function<void(ConnectionState)> cb) {
  on_connection_state_changed_cb_ = cb;
}

void NetClient::set_on_server_message_listener(
    std::function<void(pb::GameServerMessage)> cb) {
  on_server_msg_cb_ = cb;
}

void NetClient::set_state(NetClient::ConnectionState state) {
  if (state_ != state) {
    if (on_connection_state_changed_cb_) {
      on_connection_state_changed_cb_(state);
    }
    state_ = state;
  }
}