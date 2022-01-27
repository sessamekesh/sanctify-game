#ifndef SANCTIFY_GAME_SERVER_NET_NET_SERVER_H
#define SANCTIFY_GAME_SERVER_NET_NET_SERVER_H

#include <igasync/task_list.h>
#include <igcore/either.h>
#include <igcore/maybe.h>
#include <igcore/pod_vector.h>
#include <net/igameserver.h>
#include <net/igametokenexchanger.h>
#include <sanctify-game-common/proto/sanctify-net.pb.h>
#include <util/event_scheduler.h>

#include <map>
#include <memory>
#include <set>
#include <thread>
#include <vector>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

namespace sanctify {

class NetServer : public std::enable_shared_from_this<NetServer> {
 public:
  typedef websocketpp::server<websocketpp::config::asio> WSServer;
  typedef WSServer::message_ptr WSServerMsg;

 public:
  enum class ConfigError {
    InvalidPortNumber,

    MissingMessageParseTaskList,
    MissingBookkeepingTaskList,
    MissingTokenExchanger,
    MissingEventScheduler,
  };

  enum class ServerCreateError {
    InvalidConfig,
    AsioInitFailed,
  };

  struct Config {
    Config();

    std::shared_ptr<indigo::core::TaskList> MessageParseTaskList;
    std::shared_ptr<indigo::core::TaskList> BookkeepingTaskList;
    uint32_t Port;
    std::shared_ptr<IGameTokenExchanger> GameTokenExchanger;
    std::shared_ptr<EventScheduler> EventScheduler_;
    bool AllowJsonMessages;

    std::chrono::high_resolution_clock::duration UnconfirmedConnectionTimeout;

    indigo::core::PodVector<ConfigError> get_config_errors() const;
  };

  //
  // Public API
  //
 public:
  static indigo::core::Either<std::shared_ptr<NetServer>,
                              indigo::core::PodVector<ServerCreateError>>
  Create(const Config& config);
  ~NetServer();

  void register_game_server(GameId game_id,
                            std::shared_ptr<IGameServer> game_server);
  void unregister_game_server(GameId game_id);

  //
  // WS event callbacks
  //
 private:
  void setup_ws_callbacks(const Config& config);
  static void _on_open(std::shared_ptr<NetServer> net_server,
                       websocketpp::connection_hdl hdl);
  static void _on_close(std::shared_ptr<NetServer> net_server,
                        websocketpp::connection_hdl hdl);
  static void _on_message(std::shared_ptr<NetServer> net_server,
                          websocketpp::connection_hdl hdl,
                          WSServer::message_ptr msg);
  void on_open(websocketpp::connection_hdl hdl);
  void on_close(websocketpp::connection_hdl hdl);
  void on_message(websocketpp::connection_hdl hdl,
                  indigo::net::pb::GameClientMessage msg);

  //
  // Message receiver callbacks
  //
  void on_initial_connection_request_message(
      websocketpp::connection_hdl hdl,
      const indigo::net::pb::InitialConnectionRequest& request);

  //
  // Lifecycle callbacks
  //
  static void _try_kick_unconfirmed_connection(
      std::shared_ptr<NetServer> net_server, websocketpp::connection_hdl hdl);
  void try_kick_unconfirmed_connection(websocketpp::connection_hdl hdl);

  //
  // Private data members / ctor
  //
 private:
  NetServer(const Config& config);

  WSServer server_;
  std::thread server_listener_thread_;
  indigo::core::Maybe<ServerCreateError> create_error_;
  bool allow_json_messages_;

  std::shared_ptr<indigo::core::TaskList> message_parse_task_list_;
  std::shared_ptr<indigo::core::TaskList> book_keeping_task_list_;

  std::shared_ptr<IGameTokenExchanger> game_token_exchanger_;
  std::shared_ptr<EventScheduler> event_scheduler_;

  // Lookup of game server ID to game server instance running on this machine.
  // In theory, one machine can host multiple game servers - this might be worth
  //  trimming down to just one in the future (to avoid extra indirection)
  std::shared_mutex game_server_lookup_lock_;
  std::map<GameId, std::shared_ptr<IGameServer>, GameId> game_server_lookup_;

  // Set of WebSocket connections that have not negotiated a proper connection
  // yet.
  std::shared_mutex unconfirmed_connections_lock_;
  std::set<websocketpp::connection_hdl,
           std::owner_less<websocketpp::connection_hdl>>
      unconfirmed_connections_;
  std::chrono::high_resolution_clock::duration unconfirmed_connection_timeout_;
};

std::string to_string(NetServer::ConfigError err);
std::string to_string(NetServer::ServerCreateError err);

}  // namespace sanctify

#endif
