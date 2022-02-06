#ifndef SANCTIFY_GAME_CLIENT_PLATFORM_SRC_NETCLIENT_WS_CLIENT_NATIVE_H
#define SANCTIFY_GAME_CLIENT_PLATFORM_SRC_NETCLIENT_WS_CLIENT_NATIVE_H

#include <emscripten/websocket.h>
#include <netclient/ws_client_base.h>

#include <memory>

namespace sanctify {

class WsClientWeb : public WsClient,
                    public std::enable_shared_from_this<WsClientWeb> {
 public:
  WsClientWeb(std::shared_ptr<indigo::core::TaskList> message_parse_task_list,
              float healthy_time);
  ~WsClientWeb();

  void on_open(EMSCRIPTEN_WEBSOCKET_T socket);
  void on_error(EMSCRIPTEN_WEBSOCKET_T socket);
  void on_close(EMSCRIPTEN_WEBSOCKET_T socket);
  void on_message(EMSCRIPTEN_WEBSOCKET_T socket, std::string message);

  // WsClient impl
 protected:
  std::shared_ptr<indigo::core::Promise<bool>> inner_connect(
      std::string url) override;
  void send_raw_msg(std::string raw_msg) override;
  void destroy_connection() override;

 private:
  bool is_connected_;
  EMSCRIPTEN_WEBSOCKET_T ws_;
  int global_ctx_val_;
};

}  // namespace sanctify

#endif
