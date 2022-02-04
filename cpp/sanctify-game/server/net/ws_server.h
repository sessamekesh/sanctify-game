#ifndef SANCTIFY_GAME_SERVER_NET_WS_SERVER_H
#define SANCTIFY_GAME_SERVER_NET_WS_SERVER_H

#include <igasync/promise.h>
#include <igcore/bimap.h>
#include <igcore/either.h>
#include <igcore/maybe.h>
#include <igcore/pod_vector.h>
#include <net/igametokenexchanger.h>
#include <sanctify-game-common/proto/sanctify-net.pb.h>
#include <util/event_scheduler.h>

#include <map>
#include <memory>
#include <shared_mutex>
#include <thread>
#include <unordered_map>
#include <variant>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

namespace sanctify {

class WsServer : public std::enable_shared_from_this<WsServer> {
 public:
  // Possible errors that can happen to a player
  enum class WsServerCreateError {
    AsioInitFailed,
    MissingConnectPlayerListener,
    MissingReceiveMessageListener,

    ListenCallFailed,
    StartAcceptCallFailed,
  };

  // Match WebSocket ReadyState:
  // https://developer.mozilla.org/en-US/docs/Web/API/WebSocket/readyState
  enum class WsConnectionState { Open, Closed };

 public:
  // Callback types...
  using ConnectPlayerPromiseFn =
      std::function<std::shared_ptr<indigo::core::Promise<bool>>(
          const GameId&, const PlayerId&)>;
  using ReceiveMessageFromPlayerFn =
      std::function<void(const PlayerId&, pb::GameClientMessage)>;
  using OnConnectionStateChangeFn =
      std::function<void(const PlayerId&, WsConnectionState)>;

  // Creation promise type...
  using WsServerConfigurePromiseT = indigo::core::Promise<
      indigo::core::Maybe<indigo::core::PodVector<WsServerCreateError>>>;

  // Helpful shorthands...
  using WSServer = websocketpp::server<websocketpp::config::asio>;
  using WSMsgPtr = WSServer::message_ptr;

 public:
  static std::shared_ptr<WsServer> Create(
      bool allow_json_messages, uint16_t websocket_port,
      std::shared_ptr<indigo::core::TaskList> async_task_list,
      std::shared_ptr<EventScheduler> event_scheduler,
      std::shared_ptr<IGameTokenExchanger> token_exchanger,
      uint32_t unconfirmed_connection_timeout_ms);
  std::shared_ptr<WsServerConfigurePromiseT> configure_and_start_server();

  //
  // Public API
  //
  void send_message(const PlayerId& player_id, std::string data);
  void kick_player(const PlayerId& player_id);
  void shutdown();

  //
  // Receivers
  //
  void set_player_connection_verify_fn(ConnectPlayerPromiseFn player_verify_fn);
  void set_on_message_callback(ReceiveMessageFromPlayerFn cb);
  void set_on_connection_state_change(OnConnectionStateChangeFn cb);

 private:
  // WS listeners:
  static void _on_open(std::shared_ptr<WsServer> server,
                       websocketpp::connection_hdl hdl);
  void on_open(websocketpp::connection_hdl);

  static void _on_close(std::shared_ptr<WsServer> server,
                        websocketpp::connection_hdl hdl);
  void on_close(websocketpp::connection_hdl);

  static void _on_message(std::shared_ptr<WsServer> server,
                          websocketpp::connection_hdl hdl, WSMsgPtr msg);
  void on_message(websocketpp::connection_hdl hdl, WSMsgPtr msg);

 private:
  WsServer(bool allow_json_messages, uint16_t websocket_port,
           std::shared_ptr<indigo::core::TaskList> async_task_list,
           std::shared_ptr<EventScheduler> event_scheduler,
           std::shared_ptr<IGameTokenExchanger> token_exchanger,
           uint32_t unconfirmed_connection_timeout_ms);

  ConnectPlayerPromiseFn player_connection_verify_function_;
  ReceiveMessageFromPlayerFn on_message_cb_;
  OnConnectionStateChangeFn on_connection_state_change_cb_;

  indigo::core::Maybe<pb::GameClientMessage> parse_msg(
      const std::string& payload) const;

  WSServer server_;
  std::thread server_listener_thread_;
  bool allow_json_messages_;
  uint16_t websocket_port_;

  std::shared_ptr<indigo::core::TaskList> async_task_list_;
  std::shared_ptr<EventScheduler> event_scheduler_;
  std::shared_ptr<IGameTokenExchanger> token_exchanger_;

  //
  // Internals
  //

  // New connections: These are connections that have been opened, and are
  // waiting for the client to send a connection request. They will be deleted
  // if an invalid message is received from the client, or if they take too long
  // to send a connection request.
  // Key: connection. Value: cancel token for EventScheduler that kicks them
  std::shared_mutex mut_new_connections_;
  std::map<websocketpp::connection_hdl, uint32_t,
           std::owner_less<websocketpp::connection_hdl>>
      new_connections_;

  // Pending confirmation connections: These are connections that have sent
  // a connection request, but the response from the Game API has not finished
  // yet. They have a longer timeout.
  // Key: connection. Value: cancel token for EventScheduler that kicks them
  std::shared_mutex mut_pending_confirmation_connections_;
  std::map<websocketpp::connection_hdl, uint32_t,
           std::owner_less<websocketpp::connection_hdl>>
      pending_confirmation_connections_;

  // Confirmed connections - these have gone through the full connection flow
  // and are associated with a specific player.
  std::shared_mutex mut_player_connections_;
  indigo::core::Bimap<PlayerId, websocketpp::connection_hdl,
                      std::less<PlayerId>,
                      std::owner_less<websocketpp::connection_hdl>>
      player_connections_;

  // Logistics
  std::chrono::high_resolution_clock::duration unconfirmed_connection_timeout_;
};

std::string to_string(WsServer::WsServerCreateError);

}  // namespace sanctify

#endif
