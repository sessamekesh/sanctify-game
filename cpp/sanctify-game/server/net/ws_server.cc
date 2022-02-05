#include <google/protobuf/util/json_util.h>
#include <igcore/log.h>
#include <net/ws_server.h>
#include <sanctify-game-common/net/net_config.h>
#include <sanctify-game-common/proto/sanctify-net.pb.h>
#include <util/server_clock.h>

#include <chrono>

using namespace indigo;
using namespace core;
using namespace sanctify;
using namespace std::chrono_literals;

using hrclock = std::chrono::high_resolution_clock;

namespace {
const char* kLogLabel = "WsServer";

void default_connection_state_change_fn(const PlayerId& player,
                                        WsServer::WsConnectionState state) {
  // no-op, if the parent doesn't want to deal with this that's OK
}

}  // namespace

WsServer::WsServer(bool allow_json_messages, uint16_t websocket_port,
                   std::shared_ptr<indigo::core::TaskList> async_task_list,
                   std::shared_ptr<EventScheduler> event_scheduler,
                   std::shared_ptr<IGameTokenExchanger> token_exchanger,
                   uint32_t unconfirmed_connection_timeout_ms)
    : player_connection_verify_function_(nullptr),
      on_message_cb_(nullptr),
      on_connection_state_change_cb_(::default_connection_state_change_fn),
      server_(),
      server_listener_thread_(),
      allow_json_messages_(allow_json_messages),
      websocket_port_(websocket_port),
      async_task_list_(async_task_list),
      event_scheduler_(event_scheduler),
      token_exchanger_(token_exchanger),
      unconfirmed_connection_timeout_(
          std::chrono::milliseconds(unconfirmed_connection_timeout_ms)) {}

std::shared_ptr<WsServer> WsServer::Create(
    bool allow_json_messages, uint16_t websocket_port,
    std::shared_ptr<indigo::core::TaskList> async_task_list,
    std::shared_ptr<EventScheduler> event_scheduler,
    std::shared_ptr<IGameTokenExchanger> token_exchanger,
    uint32_t unconfirmed_connection_timeout_ms) {
  return std::shared_ptr<WsServer>(new WsServer(
      allow_json_messages, websocket_port, async_task_list, event_scheduler,
      token_exchanger, unconfirmed_connection_timeout_ms));
}

std::shared_ptr<WsServer::WsServerConfigurePromiseT>
WsServer::configure_and_start_server() {
  PodVector<WsServerCreateError> errors;
  if (player_connection_verify_function_ == nullptr) {
    errors.push_back(WsServerCreateError::MissingConnectPlayerListener);
  }
  if (on_message_cb_ == nullptr) {
    errors.push_back(WsServerCreateError::MissingReceiveMessageListener);
  }

  if (errors.size() > 0) {
    return WsServerConfigurePromiseT::immediate(std::move(errors));
  }

  server_.set_access_channels(websocketpp::log::alevel::all);
  server_.clear_access_channels(websocketpp::log::alevel::frame_payload);
  server_.clear_access_channels(websocketpp::log::alevel::frame_header);

  auto _this = shared_from_this();
  server_.set_open_handler(
      std::bind(WsServer::_on_open, _this, std::placeholders::_1));
  server_.set_close_handler(
      std::bind(WsServer::_on_close, _this, std::placeholders::_1));
  server_.set_message_handler(std::bind(WsServer::_on_message, _this,
                                        std::placeholders::_1,
                                        std::placeholders::_2));

  std::error_code ec;
  ec.clear();
  server_.init_asio(ec);
  if (ec) {
    errors.push_back(WsServerCreateError::AsioInitFailed);
    return WsServerConfigurePromiseT::immediate(std::move(errors));
  }

  auto rsl_promise = WsServerConfigurePromiseT::create();

  server_listener_thread_ = std::thread([_this, rsl_promise]() {
    std::error_code ec;
    ec.clear();
    _this->server_.listen(_this->websocket_port_, ec);
    if (ec) {
      PodVector<WsServerCreateError> errors;
      errors.push_back(WsServerCreateError::ListenCallFailed);
      rsl_promise->resolve(std::move(errors));
      return;
    }

    ec.clear();
    _this->server_.start_accept(ec);
    if (ec) {
      PodVector<WsServerCreateError> errors;
      errors.push_back(WsServerCreateError::StartAcceptCallFailed);
      rsl_promise->resolve(std::move(errors));
      return;
    }

    // HACK - resolve the promise a half second in the future, that should
    //  be way more than enough time for the next method to get the server
    //  into a listening state.
    _this->event_scheduler_->schedule_task(
        std::chrono::high_resolution_clock::now() + 500ms,
        _this->async_task_list_,
        [rsl_promise]() { rsl_promise->resolve(empty_maybe()); });

    Logger::log(kLogLabel)
        << "Successfully configured WebSocket server - starting loop on thread "
        << std::this_thread::get_id();

    // Start the server!
    _this->server_.run();
  });

  return rsl_promise;
}

void WsServer::_on_open(std::shared_ptr<WsServer> server,
                        websocketpp::connection_hdl hdl) {
  server->on_open(hdl);
}

void WsServer::on_open(websocketpp::connection_hdl hdl) {
  auto that = shared_from_this();
  {
    std::unique_lock<std::shared_mutex> l(mut_new_connections_);
    uint32_t cancel_key = event_scheduler_->schedule_task(
        hrclock::now() + unconfirmed_connection_timeout_, async_task_list_,
        [that, hdl, this]() {
          std::unique_lock<std::shared_mutex> l(mut_new_connections_);
          auto conn = new_connections_.find(hdl);
          if (conn == new_connections_.end()) {
            // Already handled, but there was a race condition - no-op
            return;
          }

          // TODO (sessamekesh): Close with an actual server message here
          server_.close(hdl, websocketpp::close::status::normal, "Timeout");
          new_connections_.erase(conn);
        });

    new_connections_.emplace(hdl, cancel_key);
  }
}

void WsServer::send_message(const PlayerId& player_id,
                            pb::GameServerMessage data) {
  std::shared_lock<std::shared_mutex> l(mut_player_connections_);

  auto conn_it = player_connections_.find_l(player_id);
  if (conn_it == player_connections_.end()) {
    Logger::log(kLogLabel) << "Unable to find WS connection for player "
                           << player_id.Id << " - cannot send message";
    return;
  }

  send_message(*conn_it, std::move(data), player_id);
}

void WsServer::send_message(websocketpp::connection_hdl hdl,
                            pb::GameServerMessage data,
                            indigo::core::Maybe<PlayerId> player_id) {
  // Set remaining headers
  data.set_magic_number(sanctify::kSanctifyMagicHeader);
  data.set_clock_time(ServerClock::time());
  std::string raw_data = data.SerializeAsString();

  std::error_code ec;
  ec.clear();
  server_.send(hdl, raw_data, websocketpp::frame::opcode::binary, ec);
  if (ec) {
    if (player_id.has_value()) {
      Logger::err(kLogLabel)
          << "Failed to send WS message to player " << player_id.get().Id
          << ": websocketpp error " << ec.message();
    } else {
      Logger::err(kLogLabel)
          << "Failed to send WS message to unknown player: websocketpp error "
          << ec.message();
    }
    return;
  }
}

void WsServer::kick_player(const PlayerId& player_id) {
  std::shared_lock<std::shared_mutex> l(mut_player_connections_);

  auto conn_it = player_connections_.find_l(player_id);
  if (conn_it == player_connections_.end()) {
    Logger::log(kLogLabel) << "Unable to find WS connection for player "
                           << player_id.Id << " - cannot kick!";
    return;
  }

  std::error_code ec;
  ec.clear();
  server_.close(*conn_it, websocketpp::close::status::normal,
                "Server initiated a disconnect action", ec);

  if (ec) {
    Logger::err(kLogLabel) << "Error closing connection for player "
                           << player_id.Id << ": " << ec.message();
    return;
  }

  player_connections_.erase_l(player_id);
}

void WsServer::shutdown() { server_.stop(); }

void WsServer::set_player_connection_verify_fn(
    ConnectPlayerPromiseFn player_verify_fn) {
  player_connection_verify_function_ = player_verify_fn;
}

void WsServer::set_on_message_callback(ReceiveMessageFromPlayerFn cb) {
  on_message_cb_ = cb;
}

void WsServer::set_on_connection_state_change(OnConnectionStateChangeFn fn) {
  on_connection_state_change_cb_ = fn;
}

void WsServer::_on_close(std::shared_ptr<WsServer> server,
                         websocketpp::connection_hdl hdl) {
  server->on_close(hdl);
}

void WsServer::on_close(websocketpp::connection_hdl hdl) {
  {
    std::unique_lock<std::shared_mutex> l(mut_new_connections_);
    auto it = new_connections_.find(hdl);
    if (it != new_connections_.end()) {
      event_scheduler_->cancel_task(it->second);
      new_connections_.erase(it);
    }
  }

  {
    std::unique_lock<std::shared_mutex> l(
        mut_pending_confirmation_connections_);
    auto it = pending_confirmation_connections_.find(hdl);
    if (it != pending_confirmation_connections_.end()) {
      event_scheduler_->cancel_task(it->second);
      pending_confirmation_connections_.erase(it);
    }
  }

  {
    std::unique_lock<std::shared_mutex> l(mut_player_connections_);
    auto it = player_connections_.find_r(hdl);
    if (it != player_connections_.end()) {
      if (on_connection_state_change_cb_) {
        on_connection_state_change_cb_(*it, WsConnectionState::Closed);
      }
      player_connections_.erase_r(hdl);
    }
  }
}

void WsServer::_on_message(std::shared_ptr<WsServer> server,
                           websocketpp::connection_hdl hdl, WSMsgPtr msg) {
  server->on_message(hdl, msg);
}

void WsServer::on_message(websocketpp::connection_hdl hdl, WSMsgPtr msg) {
  auto that = shared_from_this();
  // Common case - receiving a message from a connected client
  {
    std::shared_lock<std::shared_mutex> l(mut_player_connections_);
    auto it = player_connections_.find_r(hdl);
    if (it != player_connections_.end()) {
      PlayerId player_id = *it;

      async_task_list_->add_task(Task::of([that, player_id, hdl, msg]() {
        if (that->on_message_cb_) {
          auto maybe_parsed_msg = that->parse_msg(msg->get_payload());
          if (maybe_parsed_msg.is_empty()) {
            Logger::log(kLogLabel)
                << "Received improperly formatted message from "
                << player_id.Id;
            return;
          }

          pb::GameClientMessage msg = maybe_parsed_msg.move();

          that->on_message_cb_(player_id, std::move(msg));
        }
      }));

      return;
    }
  }

  // Uncommon case - new connection message (happens once per connection)
  std::shared_lock<std::shared_mutex> l(mut_new_connections_);
  auto it = new_connections_.find(hdl);
  if (it != new_connections_.end()) {
    async_task_list_->add_task(Task::of([player_id = *it, hdl, msg, that]() {
      if (that->on_message_cb_) {
        auto maybe_msg = that->parse_msg(msg->get_payload());
        if (maybe_msg.is_empty()) {
          return;
        }
        const pb::GameClientMessage& client_msg = maybe_msg.get();

        if (!client_msg.has_initial_connection_request()) {
          Logger::log(kLogLabel)
              << "Message received from unconnected player that is not an "
                 "initial connection request";
          // TODO (sessamekesh): Disconnect the player?
          return;
        }

        const auto& conn_request = client_msg.initial_connection_request();

        const std::string& token = conn_request.player_token();

        {
          std::unique_lock<std::shared_mutex> l(that->mut_new_connections_);
          auto it = that->new_connections_.find(hdl);
          if (it != that->new_connections_.end()) {
            that->event_scheduler_->cancel_task(it->second);
            that->new_connections_.erase(it);
          }
        }

        {
          std::unique_lock<std::shared_mutex> l(
              that->mut_pending_confirmation_connections_);
          uint32_t cancel_key = that->event_scheduler_->schedule_task(
              hrclock::now() + 3000ms, that->async_task_list_, [that, hdl]() {
                std::unique_lock<std::shared_mutex> l(
                    that->mut_pending_confirmation_connections_);
                auto it = that->pending_confirmation_connections_.find(hdl);
                if (it != that->pending_confirmation_connections_.end()) {
                  // TODO (sessamekesh): Close with an actual server message
                  that->server_.close(hdl, websocketpp::close::status::normal,
                                      "Timeout");
                  that->pending_confirmation_connections_.erase(it);
                }
              });
          that->pending_confirmation_connections_.emplace(hdl, cancel_key);
        }

        that->token_exchanger_->exchange(token)->on_success(
            [that, hdl = hdl](const Either<GameTokenExchangerResponse,
                                           GameTokenExchangerError>&
                                  token_exchanger_response) {
              if (token_exchanger_response.is_right()) {
                Logger::log(kLogLabel)
                    << "Token exchange request rejected: "
                    << to_string(token_exchanger_response.get_right());
                return;
              }

              const sanctify::GameTokenExchangerResponse& token_response =
                  token_exchanger_response.get_left();

              that->player_connection_verify_function_(token_response.gameId,
                                                       token_response.playerId)
                  ->on_success(
                      [that, hdl, playerId = token_response.playerId](
                          const bool& is_valid) {
                        {
                          std::unique_lock<std::shared_mutex> l(
                              that->mut_pending_confirmation_connections_);
                          auto it =
                              that->pending_confirmation_connections_.find(hdl);
                          if (it !=
                              that->pending_confirmation_connections_.end()) {
                            that->event_scheduler_->cancel_task(it->second);
                            that->pending_confirmation_connections_.erase(it);
                          }
                        }

                        if (!is_valid) {
                          // TODO (sessamekesh): actual return (proto)
                          that->server_.close(
                              hdl, websocketpp::close::status::normal,
                              "Game rejected");
                          return;
                        }

                        std::unique_lock<std::shared_mutex> l(
                            that->mut_player_connections_);
                        that->player_connections_.insert_or_update(playerId,
                                                                   hdl);

                        pb::GameServerMessage confirmation_msg{};
                        pb::InitialConnectionResponse* conn_resp =
                            confirmation_msg
                                .mutable_initial_connection_response();
                        conn_resp->set_response_type(
                            pb::InitialConnectionResponse_ResponseType::
                                InitialConnectionResponse_ResponseType_ACCEPTED);

                        that->send_message(hdl, std::move(confirmation_msg),
                                           empty_maybe{});

                        if (that->on_connection_state_change_cb_) {
                          that->on_connection_state_change_cb_(
                              playerId, WsConnectionState::Open);
                        }
                      },
                      that->async_task_list_);
            },
            that->async_task_list_);
      }
    }));
  }
}

Maybe<pb::GameClientMessage> WsServer::parse_msg(
    const std::string& payload) const {
  pb::GameClientMessage client_msg{};
  if (!client_msg.ParseFromString(payload)) {
    if (allow_json_messages_) {
      auto rsl =
          google::protobuf::util::JsonStringToMessage(payload, &client_msg);
      if (!rsl.ok()) {
        Logger::log(kLogLabel)
            << "Received player message matched neither BINARY or JSON "
               "format - parsing failed - "
            << rsl.message();
        return empty_maybe{};
      }
    } else {
      Logger::log(kLogLabel)
          << "Received WS message from new player that did not fit the "
             "GameClientMessage structure";
      return empty_maybe{};
    }
  }

  if (client_msg.magic_header() != sanctify::kSanctifyMagicHeader) {
    Logger::log(kLogLabel) << "Mismatched magic header";
    return empty_maybe{};
  }

  return client_msg;
}

std::string sanctify::to_string(WsServer::WsServerCreateError err) {
  switch (err) {
    case WsServer::WsServerCreateError::AsioInitFailed:
      return "AsioInitFailed";
    case WsServer::WsServerCreateError::MissingConnectPlayerListener:
      return "MissingConnectPlayerListener";
    case WsServer::WsServerCreateError::MissingReceiveMessageListener:
      return "MissingReceiveMessageListener";
    case WsServer::WsServerCreateError::ListenCallFailed:
      return "ListenCallFailed";
    case WsServer::WsServerCreateError::StartAcceptCallFailed:
      return "StartAcceptCallFailed";
  }

  return "";
}