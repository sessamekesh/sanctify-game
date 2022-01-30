#ifndef SANCTIFY_GAME_SERVER_NET_NET_SERVER_H
#define SANCTIFY_GAME_SERVER_NET_NET_SERVER_H

#include <igasync/promise.h>
#include <igcore/either.h>
#include <igcore/pod_vector.h>
#include <net/igametokenexchanger.h>
#include <net/ws_server.h>
#include <sanctify-game-common/proto/sanctify-net.pb.h>
#include <util/types.h>

#include <memory>

namespace sanctify {

class NetServer : public std::enable_shared_from_this<NetServer> {
 public:
  enum class ServerCreateError {
    WebsocketServerCreateError,
    WebRTCServerCreateError,
    InvalidConfigurationError,
  };

  enum class PlayerConnectionState {
    /** Player is disconnecte */
    ConnectedBasic,
    ConnectedPreferred,
    Disconnected,
    Unhealthy,
    Forbidden,
  };

 public:
  // Callback types...
  using ConnectPlayerPromiseFn =
      std::function<std::shared_ptr<indigo::core::Promise<bool>>(
          const GameId&, const PlayerId&)>;
  using ReceiveMessageFromPlayerFn =
      std::function<bool(const PlayerId&, indigo::core::RawBuffer)>;
  using OnPlayerConnectionStateChangeFn =
      std::function<void(const PlayerId&, PlayerConnectionState)>;

  // Creation promise type...
  using NetServerCreatePromiseT = indigo::core::Promise<indigo::core::Either<
      std::shared_ptr<NetServer>, indigo::core::PodVector<ServerCreateError>>>;

 public:
  static std::shared_ptr<NetServer> Create(
      std::shared_ptr<indigo::core::TaskList> async_task_list,
      std::shared_ptr<IGameTokenExchanger> game_token_exchanger,
      std::shared_ptr<EventScheduler> event_scheduler,
      uint32_t unconfirmed_connection_timeout_ms, uint16_t websocket_port,
      bool allow_json_messages);

  std::shared_ptr<NetServerCreatePromiseT> configure_and_start();

  //
  // Public API
  //
  void send_message(const PlayerId& player_id,
                    const indigo::core::RawBuffer& data);
  void kick_player(const PlayerId& player_id);
  void shutdown();

  //
  // Receivers
  //
  void set_player_connection_verify_fn(ConnectPlayerPromiseFn cb);
  void set_on_message_callback(ReceiveMessageFromPlayerFn cb);
  void set_on_state_change_callback(OnPlayerConnectionStateChangeFn cb);

  //
  // Configuration
  //
  void set_health_check_parameters(float health_ping_time_s,
                                   float unhealthy_time_s,
                                   float time_to_health_restore_s);

 private:
  NetServer(std::shared_ptr<WsServer> ws_server,
            std::shared_ptr<indigo::core::TaskList> async_task_list);

  ConnectPlayerPromiseFn player_connection_verify_function_;
  ReceiveMessageFromPlayerFn on_message_cb_;
  OnPlayerConnectionStateChangeFn on_player_state_change_;

  // TODO (sessamekesh) RTC server
  std::shared_ptr<indigo::core::TaskList> async_task_list_;
  std::shared_ptr<WsServer> ws_server_;
};

std::string to_string(const NetServer::ServerCreateError& err);

}  // namespace sanctify

#endif
