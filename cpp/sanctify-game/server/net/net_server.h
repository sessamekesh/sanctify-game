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

  /** What is the connection state for a given player ID? */
  enum class PlayerConnectionState {
    // This player is not, and has not ever been, registered against the system.
    Unknown,

    // Connected via WebSocket - messages can be sent/received, but the
    // connection may suffer from latency issues
    ConnectedBasic,

    // Connected via WebRTC - messages will have nice latency characteristics,
    // but will scale poorly with large sizes due to fragmentation/reassembly
    // and increased risk of packet loss affecting a fragment
    ConnectedPreferred,

    // No connection is active - the player has been previously registered, but
    // has disconnected for some reason. They may attempt a re-connect in the
    // future.
    Disconnected,

    // The connection is unhealthy - messages have not been received in a long
    // while, and it is suspected that the connection will be dropped. Reduce
    // the frequency of sending messages - the player will want current data
    // when they reconnect, but stuffing the queue is just wasing resources.
    Unhealthy,

    // This player has been actively booted, and will never be allowed on this
    // game server again. This is reserved for future use.
    Forbidden,
  };

 public:
  // Callback types...
  using ConnectPlayerPromiseFn =
      std::function<std::shared_ptr<indigo::core::Promise<bool>>(
          const GameId&, const PlayerId&)>;
  using ReceiveMessageFromPlayerFn =
      std::function<void(const PlayerId&, pb::GameClientMessage)>;
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
  void send_message(const PlayerId& player_id, pb::GameServerMessage data);
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
  struct PlayerState {
    PlayerConnectionState connectionState;
  };

  NetServer(std::shared_ptr<WsServer> ws_server,
            std::shared_ptr<indigo::core::TaskList> async_task_list);

  // Callbacks
  ConnectPlayerPromiseFn player_connection_verify_function_;
  ReceiveMessageFromPlayerFn on_message_cb_;
  OnPlayerConnectionStateChangeFn on_player_state_change_;

  // Connection state
  std::shared_mutex player_state_m_;
  std::map<PlayerId, PlayerState> player_state_;

  // Scheduling + servers
  std::shared_ptr<indigo::core::TaskList> async_task_list_;
  std::shared_ptr<EventScheduler> event_scheduler_;
  std::shared_ptr<WsServer> ws_server_;
};

std::string to_string(const NetServer::ServerCreateError& err);

}  // namespace sanctify

#endif
