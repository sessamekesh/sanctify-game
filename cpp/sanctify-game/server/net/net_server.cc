#include <igcore/log.h>
#include <net/net_server.h>

using namespace indigo;
using namespace core;
using namespace sanctify;

namespace {
const char* kLogLabel = "NetServer";
}

NetServer::NetServer(std::shared_ptr<WsServer> ws_server,
                     std::shared_ptr<indigo::core::TaskList> async_task_list)
    : ws_server_(ws_server), async_task_list_(async_task_list) {}

std::shared_ptr<NetServer> NetServer::Create(
    std::shared_ptr<indigo::core::TaskList> async_task_list,
    std::shared_ptr<IGameTokenExchanger> game_token_exchanger,
    std::shared_ptr<EventScheduler> event_scheduler,
    uint32_t unconfirmed_connection_timeout_ms, uint16_t websocket_port,
    bool allow_json_messages) {
  auto ws_server = WsServer::Create(
      allow_json_messages, websocket_port, async_task_list, event_scheduler,
      game_token_exchanger, unconfirmed_connection_timeout_ms);

  return std::shared_ptr<NetServer>(new NetServer(ws_server, async_task_list));
}

std::shared_ptr<NetServer::NetServerCreatePromiseT>
NetServer::configure_and_start() {
  auto that = shared_from_this();

  ws_server_->set_player_connection_verify_fn(
      player_connection_verify_function_);
  ws_server_->set_on_message_callback(
      [that](const PlayerId& player, pb::GameClientMessage data) {
        that->on_message_cb_(player, std::move(data));
      });
  ws_server_->set_on_connection_state_change(
      [that](const PlayerId& player_id, WsServer::WsConnectionState state) {
        // TODO(sessamekesh): Improve this logic when WebRTC support is present
        // as well
        if (state == WsServer::WsConnectionState::Closed) {
          that->on_player_state_change_(player_id,
                                        PlayerConnectionState::Disconnected);
          return;
        }

        if (state == WsServer::WsConnectionState::Open) {
          that->on_player_state_change_(player_id,
                                        PlayerConnectionState::ConnectedBasic);
          return;
        }
      });

  return ws_server_->configure_and_start_server()
      ->then<indigo::core::Either<std::shared_ptr<NetServer>,
                                  indigo::core::PodVector<ServerCreateError>>>(
          [that](const WsServer::WsServerConfigurePromiseT::RslType& ws_rsl)
              -> indigo::core::Either<
                  std::shared_ptr<NetServer>,
                  indigo::core::PodVector<ServerCreateError>> {
            core::PodVector<ServerCreateError> errors;

            if (ws_rsl.has_value()) {
              const PodVector<WsServer::WsServerCreateError>& errs =
                  ws_rsl.get();
              auto log = Logger::err(kLogLabel);
              log << "WebSocket Server initialization errors:\n";
              for (int i = 0; i < errs.size(); i++) {
                log << "-- " << to_string(errs[i]) << "\n";
              }
              errors.push_back(ServerCreateError::WebsocketServerCreateError);
            }

            if (errors.size() > 0) {
              return right(std::move(errors));
            }

            Logger::log(kLogLabel) << "NetServer started!";
            return left(that);
          },
          async_task_list_);
}

void NetServer::send_message(const PlayerId& player_id,
                             pb::GameServerMessage msg) {
  auto that = shared_from_this();
  async_task_list_->add_task(
      Task::of([this, that, msg = std::move(msg), player_id]() {
        std::string raw = msg.SerializeAsString();
        ws_server_->send_message(player_id, raw);
      }));
}

void NetServer::kick_player(const PlayerId& player_id) {
  // TODO (sessamekesh): Add this player to the "forbidden/kicked" list.
  ws_server_->kick_player(player_id);
}

void NetServer::set_player_connection_verify_fn(
    NetServer::ConnectPlayerPromiseFn player_verify_fn) {
  player_connection_verify_function_ = player_verify_fn;
}

void NetServer::set_on_message_callback(
    NetServer::ReceiveMessageFromPlayerFn cb) {
  on_message_cb_ = cb;
}

void NetServer::set_on_state_change_callback(
    NetServer::OnPlayerConnectionStateChangeFn cb) {
  on_player_state_change_ = cb;
}

void NetServer::set_health_check_parameters(float health_ping_time_s,
                                            float unhealthy_time_s,
                                            float time_to_health_restore_s) {
  // TODO (sessamekesh): Update internal state so that the net server has
  //  different listening behavior for healthiness/unhealthiness of the
  //  underlying connection
}

void NetServer::shutdown() { ws_server_->shutdown(); }

std::string sanctify::to_string(const NetServer::ServerCreateError& err) {
  switch (err) {
    case NetServer::ServerCreateError::WebsocketServerCreateError:
      return "WebsocketServerCreateError";
    case NetServer::ServerCreateError::WebRTCServerCreateError:
      return "WebRTCServerCreateError";
    case NetServer::ServerCreateError::InvalidConfigurationError:
      return "InvalidConfigurationError";
  }

  return "<< NetServer::ServerCreateError (unknown error) >>";
}