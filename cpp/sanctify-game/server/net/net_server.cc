#include <google/protobuf/util/json_util.h>
#include <igcore/log.h>
#include <net/net_server.h>
#include <sanctify-game-common/proto/sanctify-net.pb.h>

using namespace indigo;
using namespace core;
using namespace sanctify;

using namespace std::chrono_literals;

namespace {
const char* kLogLabel = "NetServer";

// By default, allow a client 10 seconds to connect before ejecting them.
const std::chrono::high_resolution_clock::duration
    kDefaultUnconfirmedConnectionTimeout = 10s;
}  // namespace

NetServer::Config::Config()
    : MessageParseTaskList(nullptr),
      BookkeepingTaskList(nullptr),
      Port(0u),
      UnconfirmedConnectionTimeout(0ns),
      AllowJsonMessages(false) {}

PodVector<NetServer::ConfigError> NetServer::Config::get_config_errors() const {
  PodVector<ConfigError> errs;
  if (Port == 0u) {
    errs.push_back(ConfigError::InvalidPortNumber);
  }
  if (MessageParseTaskList == nullptr) {
    errs.push_back(ConfigError::MissingMessageParseTaskList);
  }
  if (BookkeepingTaskList == nullptr) {
    errs.push_back(ConfigError::MissingBookkeepingTaskList);
  }
  if (GameTokenExchanger == nullptr) {
    errs.push_back(ConfigError::MissingTokenExchanger);
  }
  if (EventScheduler_ == nullptr) {
    errs.push_back(ConfigError::MissingEventScheduler);
  }

  return errs;
}

NetServer::NetServer(const Config& config)
    : server_(),
      server_listener_thread_(),
      create_error_(empty_maybe()),
      message_parse_task_list_(config.MessageParseTaskList),
      book_keeping_task_list_(config.BookkeepingTaskList),
      game_token_exchanger_(config.GameTokenExchanger),
      event_scheduler_(config.EventScheduler_),
      unconfirmed_connection_timeout_(
          (config.UnconfirmedConnectionTimeout.count() == 0ull)
              ? kDefaultUnconfirmedConnectionTimeout
              : config.UnconfirmedConnectionTimeout),
      allow_json_messages_(config.AllowJsonMessages) {
  server_.set_access_channels(websocketpp::log::alevel::all);
  server_.clear_access_channels(websocketpp::log::alevel::frame_payload);

  std::error_code ec;
  ec.clear();
  server_.init_asio(ec);
  if (ec) {
    create_error_ = ServerCreateError::AsioInitFailed;
    return;
  }
}

NetServer::~NetServer() {
  server_.stop();
  server_listener_thread_.join();
}

Either<std::shared_ptr<NetServer>, PodVector<NetServer::ServerCreateError>>
NetServer::Create(const Config& config) {
  PodVector<ServerCreateError> errors;
  PodVector<ConfigError> config_errors = config.get_config_errors();
  if (config_errors.size() > 0) {
    auto log = Logger::err(kLogLabel);
    log << "Configuration validation errors:\n";
    for (int i = 0; i < config_errors.size(); i++) {
      log << "-- " << to_string(config_errors[i]) << "\n";
    }
    errors.push_back(ServerCreateError::InvalidConfig);
  }

  if (errors.size() > 0) {
    return right(std::move(errors));
  }

  std::shared_ptr<NetServer> server =
      std::shared_ptr<NetServer>(new NetServer(config));
  if (server->create_error_.has_value()) {
    errors.push_back(server->create_error_.get());
    return right(std::move(errors));
  }

  server->setup_ws_callbacks(config);

  return left(std::move(server));
}

void NetServer::register_game_server(GameId game_id,
                                     std::shared_ptr<IGameServer> game_server) {
  std::unique_lock<std::shared_mutex> l(game_server_lookup_lock_);
  game_server_lookup_[game_id] = game_server;
}

void NetServer::unregister_game_server(GameId game_id) {
  std::unique_lock<std::shared_mutex> l(game_server_lookup_lock_);
  game_server_lookup_.erase(game_id);
}

void NetServer::setup_ws_callbacks(const Config& config) {
  auto _this = shared_from_this();
  server_.set_open_handler(
      std::bind(NetServer::_on_open, _this, std::placeholders::_1));
  server_.set_close_handler(
      std::bind(NetServer::_on_close, _this, std::placeholders::_1));
  server_.set_message_handler(std::bind(NetServer::_on_message, _this,
                                        std::placeholders::_1,
                                        std::placeholders::_2));

  server_.listen(config.Port);
  server_.start_accept();

  // TODO (sessamekesh): Resolve a promise when the server actually starts up
  server_listener_thread_ = std::thread([_this]() { _this->server_.run(); });
}

void NetServer::_on_open(std::shared_ptr<NetServer> net_server,
                         websocketpp::connection_hdl hdl) {
  net_server->on_open(hdl);
}

void NetServer::_on_close(std::shared_ptr<NetServer> net_server,
                          websocketpp::connection_hdl hdl) {
  net_server->on_close(hdl);
}

void NetServer::_on_message(std::shared_ptr<NetServer> net_server,
                            websocketpp::connection_hdl hdl,
                            WSServer::message_ptr msg) {
  net_server->message_parse_task_list_->add_task(
      Task::of([net_server, hdl, msg]() {
        // TODO (sessamekesh): Put rate/size limiting mechanisms either in
        // here or in the configuration (above)
        net::pb::GameClientMessage proto;
        if (msg->get_opcode() == websocketpp::frame::opcode::BINARY) {
          if (!proto.ParseFromString(msg->get_payload())) {
            return;
          }
        } else if (net_server->allow_json_messages_) {
          google::protobuf::util::JsonParseOptions options;
          options.ignore_unknown_fields = false;
          options.case_insensitive_enum_parsing = false;

          auto rsl = google::protobuf::util::JsonStringToMessage(
              msg->get_payload(), &proto, options);
          if (!rsl.ok()) {
            Logger::log(kLogLabel)
                << "Invalid JSON formatted message received: "
                << msg->get_payload();
            return;
          }
        }

        // If this point is reached, the message was successfully parsed - pass
        // it back to the net server server
        net_server->book_keeping_task_list_->add_task(
            Task::of([net_server, proto = std::move(proto), hdl]() {
              net_server->on_message(hdl, std::move(proto));
            }));
      }));
}

void NetServer::on_open(websocketpp::connection_hdl hdl) {
  {
    std::unique_lock<std::shared_mutex> l(unconfirmed_connections_lock_);
    unconfirmed_connections_.emplace(hdl);
  }

  auto _that = shared_from_this();
  event_scheduler_->schedule_task(
      std::chrono::high_resolution_clock::now() +
          unconfirmed_connection_timeout_,
      book_keeping_task_list_,
      std::bind(NetServer::_try_kick_unconfirmed_connection, _that, hdl));
}

void NetServer::on_close(websocketpp::connection_hdl hdl) {
  {
    std::unique_lock<std::shared_mutex> l(unconfirmed_connections_lock_);
    unconfirmed_connections_.erase(hdl);
  }
}

void NetServer::on_message(websocketpp::connection_hdl hdl,
                           net::pb::GameClientMessage msg) {
  // TODO (sessamekesh): handle header

  switch (msg.msg_body_case()) {
    case net::pb::GameClientMessage::MsgBodyCase::kInitialConnectionRequest:
      on_initial_connection_request_message(hdl,
                                            msg.initial_connection_request());
      break;
    default:
      return;
  }
}

void NetServer::on_initial_connection_request_message(
    websocketpp::connection_hdl hdl,
    const indigo::net::pb::InitialConnectionRequest& request) {
  // Early out - if this connection is already confirmed, abort (this is a dup)
  // No lock is required here because unconfirmed connections will be written
  //  exactly twice: once before the socket has the chance to send a message,
  //  and once after the message is removed. Check again in a synchronized block
  //  below as well to avoid a race condition.
  if (unconfirmed_connections_.count(hdl) == 0u) {
    return;
  }

  // TODO (sessamekesh): remove player from "unconfirmed" to "validating" state

  auto _that = shared_from_this();
  game_token_exchanger_->exchange(request.player_token())
      ->on_success(
          [_that, hdl](const Either<GameTokenExchangerResponse,
                                    GameTokenExchangerError>& resp) {
            if (resp.is_right()) {
              Logger::err(kLogLabel) << "Failed to exchange game token: "
                                     << to_string(resp.get_right());
              return;
            }

            const GameTokenExchangerResponse& token = resp.get_left();
            std::shared_ptr<IGameServer> game_server;
            {
              std::shared_lock<std::shared_mutex> l(
                  _that->game_server_lookup_lock_);
              auto it = _that->game_server_lookup_.find(token.gameId);
              if (it != _that->game_server_lookup_.end()) {
                game_server = it->second;
              }
            }

            if (game_server != nullptr) {
              game_server->add_player_to_game(token.playerId)
                  ->on_success(
                      [_that, hdl, token](const bool& player_added) {
                        if (!player_added) {
                          Logger::err(kLogLabel)
                              << "Failed to add player " << token.playerId.Id
                              << " to game " << token.gameId.Id;
                          return;
                        }

                        Logger::log(kLogLabel)
                            << "Player " << token.playerId.Id
                            << " added to game " << token.gameId.Id;
                        {
                          std::unique_lock<std::shared_mutex> l(
                              _that->unconfirmed_connections_lock_);
                          _that->unconfirmed_connections_.erase(hdl);
                        }
                      },
                      _that->book_keeping_task_list_);
            }
          },
          book_keeping_task_list_);
}

void NetServer::_try_kick_unconfirmed_connection(
    std::shared_ptr<NetServer> net_server, websocketpp::connection_hdl hdl) {
  net_server->try_kick_unconfirmed_connection(hdl);
}

void NetServer::try_kick_unconfirmed_connection(
    websocketpp::connection_hdl hdl) {
  std::unique_lock<std::shared_mutex> l(unconfirmed_connections_lock_);
  if (unconfirmed_connections_.count(hdl) > 0) {
    Logger::log(kLogLabel) << "Kicked a WebSocket connection for not "
                              "performing handshake fast enough";
    server_.close(hdl, websocketpp::close::status::normal,
                  "Connection timed out");
    unconfirmed_connections_.erase(hdl);
  }
}

std::string sanctify::to_string(NetServer::ConfigError err) {
  switch (err) {
    case NetServer::ConfigError::InvalidPortNumber:
      return "Invalid port number";
    case NetServer::ConfigError::MissingMessageParseTaskList:
      return "Missing message parse task list";
    case NetServer::ConfigError::MissingBookkeepingTaskList:
      return "Missing bookkeping parse task list";
    case NetServer::ConfigError::MissingTokenExchanger:
      return "Missing game token exchanger";
    case NetServer::ConfigError::MissingEventScheduler:
      return "Missing event scheduler utility";
  }
  return "<< Unknown NetServer::ConfigError >>";
}

std::string sanctify::to_string(NetServer::ServerCreateError err) {
  switch (err) {
    case NetServer::ServerCreateError::InvalidConfig:
      return "Invalid configuration (check logs)";
    case NetServer::ServerCreateError::AsioInitFailed:
      return "Failed to initialize Asio";
  }

  return "<< Unknown NetServer::ServerCreateError >>";
}