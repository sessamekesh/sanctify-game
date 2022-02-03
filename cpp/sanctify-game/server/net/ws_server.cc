#include <google/protobuf/util/json_util.h>
#include <igcore/log.h>
#include <net/ws_server.h>
#include <sanctify-game-common/net/net_config.h>
#include <sanctify-game-common/proto/sanctify-net.pb.h>

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
  {
    std::unique_lock<std::shared_mutex> l(connection_state_lock_);
    if (connections_.count(hdl) > 0) {
      Logger::err(kLogLabel) << "Handle on_open callback received for existing "
                                "connection - skipping";
      return;
    }

    connections_.emplace(hdl,
                         UnconfirmedConnection{hrclock::now(), empty_maybe()});
  }

  auto _this = shared_from_this();
  event_scheduler_->schedule_task(
      hrclock::now() + unconfirmed_connection_timeout_, async_task_list_,
      [_this, hdl, this]() {
        OnConnectionDeadlineReachedVisitor visitor{hdl};

        std::unique_lock<std::shared_mutex> l(connection_state_lock_);
        auto conn = connections_.find(hdl);
        if (conn == connections_.end()) {
          // Connection was already terminated, no-op
          return;
        }

        const ConnectionState& connection = conn->second;
        OnConnectionDeadlineReachedVisitor::Action action =
            std::visit(visitor, connection);

        switch (action) {
          case OnConnectionDeadlineReachedVisitor::Action::NoOp:
            return;
          case OnConnectionDeadlineReachedVisitor::Action::Disconnect:
            connections_.erase(hdl);
            return;
        }

        Logger::err(kLogLabel)
            << "OnConnectionDeadlineReachedVisitor::Action "
               "switch was not complete - missed case "
            << (uint8_t)action << " - destroying connection to be safe!";
        connections_.erase(hdl);
      });
}

WsServer::OnConnectionDeadlineReachedVisitor::Action
WsServer::OnConnectionDeadlineReachedVisitor::operator()(
    const WsServer::UnconfirmedConnection& conn) {
  if (conn.TokenReceivedTime.has_value()) {
    // This connection is in the process of being verified, do not kick.
    return Action::NoOp;
  }

  // This connection has timed out without being verified - kick it!
  return Action::Disconnect;
}

WsServer::OnConnectionDeadlineReachedVisitor::Action
WsServer::OnConnectionDeadlineReachedVisitor::operator()(
    const WsServer::HealthyConnection& conn) {
  // Always a no-op for healthy connections
  // TODO (sessamekesh): Cancel the operation when the token is negotiated!
  return Action::NoOp;
}

void WsServer::send_message(const PlayerId& player_id,
                            const indigo::core::RawBuffer& data) {
  std::shared_lock<std::shared_mutex> l(connected_players_lock_);
  auto conn_it = connected_players_.find(player_id);
  if (conn_it == connected_players_.end()) {
    Logger::log(kLogLabel) << "Unable to find WS connection for player "
                           << player_id.Id << " - cannot send message";
    return;
  }

  std::shared_lock<std::shared_mutex> l2(connection_state_lock_);
  auto state_it = connections_.find(conn_it->second);
  if (state_it == connections_.end()) {
    Logger::log(kLogLabel)
        << "Connection found for player " << player_id.Id
        << ", but connection state was not registered - cannot send message";
    return;
  }

  SendGameMessageVisitor visitor{conn_it->second, &server_, &data,
                                 websocketpp::frame::opcode::binary};
  std::visit(visitor, state_it->second);
}

void WsServer::SendGameMessageVisitor::operator()(
    WsServer::UnconfirmedConnection& conn) {
  // This case should be impossible!
  Logger::log(kLogLabel) << "Attempting to send a game message to an "
                            "unconfirmed visitor! This should be impossible";
  return;
}

void WsServer::SendGameMessageVisitor::operator()(
    WsServer::HealthyConnection& conn) {
  std::error_code ec;
  ec.clear();
  server->send(hdl, data->get(), data->size(), op_code, ec);
  if (ec) {
    Logger::err(kLogLabel) << "Failed to send message: " << ec.message();
    return;
  }

  std::unique_lock<std::shared_mutex> l(conn.last_message_sent_time_lock_);
  conn.LastMessageSentTime = hrclock::now();
}

void WsServer::disconnect_websocket(const PlayerId& player_id) {
  std::unique_lock<std::shared_mutex> l(connected_players_lock_);
  auto conn_it = connected_players_.find(player_id);
  if (conn_it == connected_players_.end()) {
    Logger::log(kLogLabel) << "Unable to find WS connection for player "
                           << player_id.Id << " - cannot send message";
    return;
  }

  std::unique_lock<std::shared_mutex> l2(connection_state_lock_);
  auto state_it = connections_.find(conn_it->second);
  if (state_it == connections_.end()) {
    Logger::log(kLogLabel)
        << "Connection found for player " << player_id.Id
        << ", but connection state was not registered - cannot send message";
    return;
  }

  std::error_code ec;
  ec.clear();
  server_.close(conn_it->second, websocketpp::close::status::normal,
                "Server initiated a disconnect action", ec);

  if (ec) {
    Logger::err(kLogLabel) << "Error closing connection for player "
                           << player_id.Id << ": " << ec.message();
    return;
  }

  connections_.erase(state_it);
  connected_players_.erase(conn_it);
}

void WsServer::shutdown_server() { server_.stop(); }

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
  std::unique_lock<std::shared_mutex> l(connection_state_lock_);
  auto connection_it = connections_.find(hdl);
  if (connection_it == connections_.end()) {
    return;
  }

  DisconnectWsVisitor visitor{this};

  std::visit(visitor, connection_it->second);

  connections_.erase(connection_it);
}

void WsServer::DisconnectWsVisitor::operator()(UnconfirmedConnection& conn) {}

void WsServer::DisconnectWsVisitor::operator()(HealthyConnection& conn) {
  server->on_connection_state_change_cb_(conn.playerId,
                                         WsServer::WsConnectionState::Closed);
}

void WsServer::_on_message(std::shared_ptr<WsServer> server,
                           websocketpp::connection_hdl hdl, WSMsgPtr msg) {
  server->on_message(hdl, msg);
}

void WsServer::on_message(websocketpp::connection_hdl hdl, WSMsgPtr msg) {
  std::shared_lock<std::shared_mutex> l(connection_state_lock_);
  auto connection_it = connections_.find(hdl);
  if (connection_it == connections_.end()) {
    return;
  }

  OnMessageReceivedVisitor visitor{hdl, this, msg};
  std::visit(visitor, connection_it->second);
}

void WsServer::OnMessageReceivedVisitor::operator()(
    UnconfirmedConnection& conn) {
  // TODO (sessamekesh): Try to parse, then exchange the token
  pb::GameClientMessage client_msg{};

  if (!client_msg.ParseFromString(msg->get_payload())) {
    if (server->allow_json_messages_) {
      auto rsl = google::protobuf::util::JsonStringToMessage(msg->get_payload(),
                                                             &client_msg);
      if (!rsl.ok()) {
        Logger::log(kLogLabel)
            << "Received player message matched neither BINARY or JSON format "
               "parsing failed as well - "
            << rsl.message();
        return;
      }
    } else {
      Logger::log(kLogLabel)
          << "Received WS message from new player did not fit "
             "the GameClientMessage structure";
      return;
    }
  }

  if (client_msg.magic_header() != sanctify::kSanctifyMagicHeader) {
    Logger::log(kLogLabel) << "Mismatched magic header from unconnected player";
    return;
  }

  if (!client_msg.has_initial_connection_request()) {
    Logger::log(kLogLabel) << "Message form unconnected player is not an "
                              "initial connection request";
    return;
  }

  const auto& conn_request = client_msg.initial_connection_request();

  const std::string& token = conn_request.player_token();

  auto that = server->shared_from_this();
  // TODO (sessamekesh): Token exchanger should have rate limiting per
  // connection (and in general)
  server->token_exchanger_->exchange(token)->on_success(
      [that, hdl = hdl](
          const indigo::core::Either<GameTokenExchangerResponse,
                                     GameTokenExchangerError>& response) {
        if (response.is_right()) {
          Logger::log(kLogLabel) << "Token exchange request rejected: "
                                 << to_string(response.get_right());
          return;
        }

        const sanctify::GameTokenExchangerResponse& token_response =
            response.get_left();

        that->player_connection_verify_function_(token_response.gameId,
                                                 token_response.playerId)
            ->on_success(
                [that, hdl,
                 playerId = token_response.playerId](const bool& is_valid) {
                  RawBuffer maybe_dat(nullptr, 0, false);
                  {
                    std::unique_lock<std::shared_mutex> l(
                        that->connection_state_lock_);
                    auto conn_it = that->connections_.find(hdl);
                    if (conn_it != that->connections_.end()) {
                      if (std::holds_alternative<UnconfirmedConnection>(
                              conn_it->second)) {
                        that->connections_.erase(conn_it);
                        that->connections_.emplace(std::make_pair(
                            hdl, HealthyConnection(playerId, hrclock::now())));

                        std::unique_lock<std::shared_mutex> l2(
                            that->connected_players_lock_);
                        that->connected_players_.emplace(playerId, hdl);

                        that->on_connection_state_change_cb_(
                            playerId, WsConnectionState::Open);

                        pb::GameServerMessage response;
                        response.set_magic_number(
                            sanctify::kSanctifyMagicHeader);
                        pb::InitialConnectionResponse* init_resp =
                            response.mutable_initial_connection_response();
                        init_resp->set_response_type(
                            pb::InitialConnectionResponse_ResponseType::
                                InitialConnectionResponse_ResponseType_ACCEPTED);

                        std::string msg = response.SerializeAsString();

                        that->server_.send(hdl, msg,
                                           websocketpp::frame::opcode::binary);
                      }
                    }
                  }
                },
                that->async_task_list_);
      },
      server->async_task_list_);
}

void WsServer::OnMessageReceivedVisitor::operator()(HealthyConnection& conn) {
  RawBuffer data(msg->get_payload().size());
  memcpy(data.get(), &msg->get_payload()[0], msg->get_payload().size());
  server->on_message_cb_(conn.playerId, std::move(data));
}

WsServer::HealthyConnection::HealthyConnection(
    const WsServer::HealthyConnection& o)
    : ConnectionEstablishedTime(o.ConnectionEstablishedTime),
      LastMessageSentTime(o.LastMessageSentTime),
      playerId(o.playerId) {}

WsServer::HealthyConnection& WsServer::HealthyConnection::operator=(
    const WsServer::HealthyConnection& o) {
  ConnectionEstablishedTime = o.ConnectionEstablishedTime;
  LastMessageSentTime = o.LastMessageSentTime;
  playerId = o.playerId;
  return *this;
}

std::string sanctify::to_string(WsServer::WsServerCreateError err) {
  switch (err) {
    case WsServer::WsServerCreateError::AsioInitFailed:
      return "AsioInitFailed";
    case WsServer::WsServerCreateError::MissingReceiveMessageListener:
      return "MissingReceiveMessageListener";
    case WsServer::WsServerCreateError::MissingConnectPlayerListener:
      return "MissingConnectPlayerListener";
    case WsServer::WsServerCreateError::ListenCallFailed:
      return "ListenCallFailed";
    case WsServer::WsServerCreateError::StartAcceptCallFailed:
      return "StartAcceptCallFailed";
  }
}
