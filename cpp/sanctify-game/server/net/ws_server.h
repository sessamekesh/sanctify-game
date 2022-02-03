#ifndef SANCTIFY_GAME_SERVER_NET_WS_SERVER_H
#define SANCTIFY_GAME_SERVER_NET_WS_SERVER_H

#include <igasync/promise.h>
#include <igcore/either.h>
#include <igcore/pod_vector.h>
#include <net/igametokenexchanger.h>
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
      std::function<bool(const PlayerId&, indigo::core::RawBuffer)>;
  using OnConnectionStateChangeFn =
      std::function<void(const PlayerId&, WsConnectionState)>;

  // Creation promise type...
  using WsServerConfigurePromiseT = indigo::core::Promise<
      indigo::core::Maybe<indigo::core::PodVector<WsServerCreateError>>>;

  // Helpful shorthands...
  using WSServer = websocketpp::server<websocketpp::config::asio>;
  using WSMsgPtr = WSServer::message_ptr;

 private:
  // Possible internal connection states (informs how messages are processed...)
  struct UnconfirmedConnection {
    UnconfirmedConnection(const UnconfirmedConnection&) = default;
    UnconfirmedConnection& operator=(const UnconfirmedConnection&) = default;
    UnconfirmedConnection(UnconfirmedConnection&&) = default;
    UnconfirmedConnection& operator=(UnconfirmedConnection&&) = default;

    std::chrono::high_resolution_clock::time_point ConnectionMadeTime;
    indigo::core::Maybe<std::chrono::high_resolution_clock::time_point>
        TokenReceivedTime;
  };

  // TODO (sessamekesh): move this to a class since it has locking primitives
  // (should do that for other connection types too)
  struct HealthyConnection {
    HealthyConnection(PlayerId player_id,
                      std::chrono::high_resolution_clock::time_point now)
        : playerId(player_id),
          ConnectionEstablishedTime(now),
          LastMessageSentTime(now) {}

    std::chrono::high_resolution_clock::time_point ConnectionEstablishedTime;

    std::shared_mutex last_message_sent_time_lock_;
    std::chrono::high_resolution_clock::time_point LastMessageSentTime;

    PlayerId playerId;

    HealthyConnection(HealthyConnection&&) = default;
    HealthyConnection& operator=(HealthyConnection&&) = default;

    HealthyConnection(const HealthyConnection& o);
    HealthyConnection& operator=(const HealthyConnection&);
  };

  struct OnMessageReceivedVisitor {
    websocketpp::connection_hdl hdl;
    WsServer* server;
    WSMsgPtr msg;

    void operator()(UnconfirmedConnection& conn);
    void operator()(HealthyConnection& conn);
  };

  struct OnConnectionDeadlineReachedVisitor {
    enum class Action { NoOp, Disconnect };

    websocketpp::connection_hdl hdl;

    Action operator()(const UnconfirmedConnection& conn);
    Action operator()(const HealthyConnection& conn);
  };

  struct SendGameMessageVisitor {
    websocketpp::connection_hdl hdl;
    WSServer* server;
    const indigo::core::RawBuffer* const data;
    websocketpp::frame::opcode::value op_code;

    void operator()(UnconfirmedConnection& conn);
    void operator()(HealthyConnection& conn);
  };

  struct DisconnectWsVisitor {
    WsServer* server;

    void operator()(UnconfirmedConnection& conn);
    void operator()(HealthyConnection& conn);
  };

  using ConnectionState =
      std::variant<UnconfirmedConnection, HealthyConnection>;

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
  void send_message(const PlayerId& player_id,
                    const indigo::core::RawBuffer& data);
  void disconnect_websocket(const PlayerId& player_id);
  void shutdown_server();

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

  WSServer server_;
  std::thread server_listener_thread_;
  bool allow_json_messages_;
  uint16_t websocket_port_;

  std::shared_ptr<indigo::core::TaskList> async_task_list_;
  std::shared_ptr<EventScheduler> event_scheduler_;
  std::shared_ptr<IGameTokenExchanger> token_exchanger_;

  // Internals
  std::shared_mutex connection_state_lock_;
  std::map<websocketpp::connection_hdl, ConnectionState,
           std::owner_less<websocketpp::connection_hdl>>
      connections_;
  std::chrono::high_resolution_clock::duration unconfirmed_connection_timeout_;

  std::shared_mutex connected_players_lock_;
  std::map<PlayerId, websocketpp::connection_hdl> connected_players_;
};

std::string to_string(WsServer::WsServerCreateError);

}  // namespace sanctify

#endif
