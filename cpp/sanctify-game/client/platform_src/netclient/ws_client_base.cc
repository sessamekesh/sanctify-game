#include <igcore/log.h>
#include <netclient/ws_client_base.h>
#include <sanctify-game-common/net/net_config.h>

using namespace sanctify;
using namespace indigo;
using namespace core;

namespace {
const char* kLogLabel = "WsClient";
}

WsClient::WsClient(std::shared_ptr<TaskList> message_parse_task_list,
                   float healthy_time)
    : message_parse_task_list_(message_parse_task_list),
      clock_(0.f),
      healthy_time_(healthy_time),
      state_(ConnectionState::None),
      remaining_healthy_time_(0.f) {}

void WsClient::set_on_connection_state_changed(
    std::function<void(ConnectionState)> cb) {
  on_connection_state_changed_cb_ = cb;
}

void WsClient::set_on_server_message(
    std::function<void(pb::GameServerMessage)> cb) {
  on_server_msg_cb_ = cb;
}

std::shared_ptr<Promise<bool>> WsClient::connect(std::string hostname,
                                                 uint16_t port, bool use_wss,
                                                 std::string game_token) {
  set_state(ConnectionState::None);
  destroy_connection();

  game_token_ = game_token;

  std::string ws_url = hostname + ":" + std::to_string(port);
  if (use_wss) {
    ws_url = "wss://" + ws_url;
  } else {
    ws_url = "ws://" + ws_url;
  }

  on_connect_promise_ = inner_connect(ws_url);

  on_connect_promise_->on_success(
      [this, game_token](const bool& success) {
        if (!success) {
          this->set_state(ConnectionState::MisconfiguredConnection);
        }
      },
      message_parse_task_list_);

  return on_connect_promise_;
}

void WsClient::send_message(pb::GameClientMessage msg) {
  message_parse_task_list_->add_task(Task::of([msg = std::move(msg), this]() {
    this->send_raw_msg(msg.SerializeAsString());
  }));
}

void WsClient::recv_raw_msg(std::string raw_msg) {
  message_parse_task_list_->add_task(
      Task::of([raw_msg = std::move(raw_msg), this]() {
        pb::GameServerMessage msg;
        if (!msg.ParseFromString(raw_msg)) {
          return;
        }

        this->recv_msg(std::move(msg));
      }));
}

void WsClient::recv_msg(pb::GameServerMessage msg) {
  switch (state_) {
    case ConnectionState::New:
      recv_connection_msg(std::move(msg));
      return;
    case ConnectionState::Unhealthy:
      set_state(ConnectionState::Healthy);
      [[fallthrough]];
    case ConnectionState::Healthy:
      recv_game_msg(std::move(msg));
      return;
    default:
      Logger::log(kLogLabel)
          << "Received message from unexpected state " << (uint32_t)state_;
  }
}

void WsClient::recv_connection_msg(pb::GameServerMessage msg) {
  if (msg.magic_number() != sanctify::kSanctifyMagicHeader) {
    set_state(ConnectionState::MisconfiguredConnection);
    return;
  }

  if (!msg.has_initial_connection_response()) {
    Logger::log(kLogLabel)
        << "Expecting initial connection response, skipping message...";
    return;
  }

  const pb::InitialConnectionResponse& connection_response =
      msg.initial_connection_response();

  switch (connection_response.response_type()) {
    case pb::InitialConnectionResponse::ResponseType::
        InitialConnectionResponse_ResponseType_ACCEPTED:
      set_state(ConnectionState::Healthy);
      on_connect_promise_->resolve(true);
      break;
    default:
      set_state(ConnectionState::MisconfiguredConnection);
      on_connect_promise_->resolve(false);
      break;
  }

  on_connect_promise_ = nullptr;
}

void WsClient::recv_game_msg(pb::GameServerMessage msg) {
  remaining_healthy_time_ = healthy_time_;
  on_server_msg_cb_(std::move(msg));
}

void WsClient::tick_clock(float dt) {
  remaining_healthy_time_ -= dt;
  if (remaining_healthy_time_ < 0.f && state_ != ConnectionState::Unhealthy) {
    set_state(ConnectionState::Unhealthy);
  }
}

void WsClient::on_connect() {
  switch (state_) {
    case ConnectionState::None:
      set_state(ConnectionState::New);
      {
        pb::GameClientMessage msg;
        msg.set_magic_header(sanctify::kSanctifyMagicHeader);
        pb::InitialConnectionRequest* initial_connection_request =
            msg.mutable_initial_connection_request();
        initial_connection_request->set_player_token(game_token_);
        this->send_message(std::move(msg));
      }
      break;
    default:
      // noop
      break;
  }
}

void WsClient::on_error(std::string err) {
  if (on_connect_promise_) {
    on_connect_promise_->resolve(false);
    on_connect_promise_ = nullptr;
  }

  Logger::log(kLogLabel) << "WebSocket error: " << err;
  set_state(ConnectionState::MisconfiguredConnection);
}

void WsClient::on_disconnect() {
  if (on_connect_promise_) {
    on_connect_promise_->resolve(false);
    on_connect_promise_ = nullptr;
  }

  set_state(ConnectionState::None);
}

void WsClient::set_state(WsClient::ConnectionState state) {
  if (state_ != state) {
    state_ = state;
    on_connection_state_changed_cb_(state);
  }
}

bool WsClient::is_connected() {
  switch (state_) {
    case ConnectionState::Healthy:
    case ConnectionState::Unhealthy:
      return true;

    default:
      return false;
  }
}