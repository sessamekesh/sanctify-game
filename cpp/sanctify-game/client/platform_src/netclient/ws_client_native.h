#ifndef SANCTIFY_GAME_CLIENT_PLATFORM_SRC_NETCLIENT_WS_CLIENT_NATIVE_H
#define SANCTIFY_GAME_CLIENT_PLATFORM_SRC_NETCLIENT_WS_CLIENT_NATIVE_H

#include <igcore/maybe.h>
#include <netclient/ws_client_base.h>

#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>

namespace sanctify {

class WsClientNative : public WsClient,
                       public std::enable_shared_from_this<WsClientNative> {
 public:
  using WSClient = websocketpp::client<websocketpp::config::asio_client>;
  using WSMsgPtr = WSClient::message_ptr;

 private:
  // WS listeners:
  static void _on_open(std::shared_ptr<WsClientNative> client,
                       websocketpp::connection_hdl hdl);
  void on_open(websocketpp::connection_hdl hdl);

  static void _on_fail(std::shared_ptr<WsClientNative> client,
                       websocketpp::connection_hdl hdl);
  void on_fail(websocketpp::connection_hdl hdl);

  static void _on_message(std::shared_ptr<WsClientNative> client,
                          websocketpp::connection_hdl hdl, WSMsgPtr msg_ptr);
  void on_message(websocketpp::connection_hdl hdl, WSMsgPtr msg_ptr);

  static void _on_close(std::shared_ptr<WsClientNative> client,
                        websocketpp::connection_hdl hdl);
  void on_close(websocketpp::connection_hdl hdl);

 public:
  WsClientNative(
      std::shared_ptr<indigo::core::TaskList> message_parse_task_list,
      float healthy_time);

  // WsClient impl
 protected:
  std::shared_ptr<indigo::core::Promise<bool>> inner_connect(
      std::string url) override;
  void send_raw_msg(std::string raw_msg) override;
  void destroy_connection() override;

 private:
  WSClient client_;
  struct WsInternal {
    websocketpp::connection_hdl connection;
  };
  indigo::core::Maybe<WsInternal> ws_;

  bool is_running_;
};

}  // namespace sanctify

#endif
