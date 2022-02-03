#ifndef SANCTIFY_GAME_CLIENT_PLATFORM_SRC_NETCLIENT_WS_CLIENT_NATIVE_H
#define SANCTIFY_GAME_CLIENT_PLATFORM_SRC_NETCLIENT_WS_CLIENT_NATIVE_H

#include <netclient/ws_client_base.h>

#include <rtc/rtc.hpp>

namespace sanctify {

class WsClientNative : public WsClient,
                       public std::enable_shared_from_this<WsClientNative> {
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
  rtc::WebSocket ws_;
};

}  // namespace sanctify

#endif
