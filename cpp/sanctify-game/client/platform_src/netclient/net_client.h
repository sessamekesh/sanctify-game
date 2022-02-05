#ifndef SANCTIFY_GAME_CLIENT_PLATFORM_SRC_NETCLIENT_NET_CLIENT_H
#define SANCTIFY_GAME_CLIENT_PLATFORM_SRC_NETCLIENT_NET_CLIENT_H

#include <netclient/ws_client_base.h>

/**
 * NetClient implementation - takes a WebSocket server (which may be a native or
 * web implementation).
 */

namespace sanctify {

class NetClient : public std::enable_shared_from_this<NetClient> {
 public:
  enum class ConnectionState {
    // No connection is active right now
    Disconnected,

    // The connection is malformed, and cannot succeed with this configuration
    Malformed,

    // The connection is unhealthy, but still present.
    Unhealthy,

    // The connection is formed under non-ideal (TCP) conditions
    Basic,

    // The connection is formed under ideal (RTC) conditions
    Full,
  };

 public:
  NetClient(std::shared_ptr<indigo::core::TaskList> message_parse_task_list,
            float healthy_time);

  NetClient(std::shared_ptr<WsClient> ws_client_impl);

  void tick_clock(float dt);
  std::shared_ptr<indigo::core::Promise<bool>> connect(std::string hostname,
                                                       uint16_t ws_port,
                                                       bool use_wss,
                                                       std::string game_token);

  void set_connection_state_changed_listener(
      std::function<void(ConnectionState)> cb);
  void set_on_server_message_listener(
      std::function<void(pb::GameServerMessage)> cb);

  void send_message(pb::GameClientMessage msg);

  ConnectionState get_connection_state() const;

 private:
  void set_state(ConnectionState state);

  ConnectionState state_;

  std::shared_ptr<WsClient> ws_client_;

  std::function<void(ConnectionState)> on_connection_state_changed_cb_;
  std::function<void(pb::GameServerMessage)> on_server_msg_cb_;
};

std::string to_string(NetClient::ConnectionState state);

}  // namespace sanctify

#endif
