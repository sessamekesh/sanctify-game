#ifndef SANCTIFY_GAME_CLIENT_PLATFORM_SRC_NETCLIENT_WS_CLIENT_BASE_H
#define SANCTIFY_GAME_CLIENT_PLATFORM_SRC_NETCLIENT_WS_CLIENT_BASE_H

/**
 * WebSocket client base - there is both a native and web implementation which
 * are different, but can at least conform to the same interface.
 *
 * There are also bookkeeping things that are the same between client/server
 */

#include <igasync/promise.h>
#include <igasync/task_list.h>
#include <sanctify-game-common/proto/sanctify-net.pb.h>

#include <functional>

namespace sanctify {

class WsClient {
 public:
  enum class ConnectionState {
    // No connection at all - no attempt has been made to connect with a server
    None,

    // Connection has been attempted, and is in the process of establishment
    New,

    // Connection is invalid, and a new connection must be made before
    //  attempting any server communication.
    MisconfiguredConnection,

    // Connection hasn't been formally dropped, but is unhealthy because no
    //  message has been received in too long.
    Unhealthy,

    // Connection is healthy and ready to send/receive messages
    Healthy,
  };

 public:
  WsClient(std::shared_ptr<indigo::core::TaskList> message_parse_task_list,
           float healthy_time);

  void set_on_connection_state_changed(std::function<void(ConnectionState)> cb);
  void set_on_server_message(std::function<void(pb::GameServerMessage)> cb);

  std::shared_ptr<indigo::core::Promise<bool>> connect(std::string hostname,
                                                       uint16_t port,
                                                       bool use_wss,
                                                       std::string game_token);

  ConnectionState state() const { return state_; }
  virtual void destroy_connection() = 0;

  void send_message(pb::GameClientMessage msg);
  void tick_clock(float dt);

  bool is_connected();

 protected:
  void recv_raw_msg(std::string raw_msg);
  virtual void send_raw_msg(std::string raw_msg) = 0;
  virtual std::shared_ptr<indigo::core::Promise<bool>> inner_connect(
      std::string url) = 0;

  void on_connect();
  void on_error(std::string err);
  void on_disconnect();

 private:
  void recv_msg(pb::GameServerMessage msg);
  void recv_connection_msg(pb::GameServerMessage msg);
  void recv_game_msg(pb::GameServerMessage msg);

  void set_state(ConnectionState state);

  ConnectionState state_;
  float clock_;
  const float healthy_time_;
  float remaining_healthy_time_;

  std::function<void(ConnectionState)> on_connection_state_changed_cb_;
  std::function<void(pb::GameServerMessage)> on_server_msg_cb_;

  std::shared_ptr<indigo::core::TaskList> message_parse_task_list_;

  std::shared_ptr<indigo::core::Promise<bool>> on_connect_promise_;
  std::string game_token_;
};

}  // namespace sanctify

#endif
