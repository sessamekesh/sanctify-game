#include <netclient/ws_client_base.h>
#include <netclient/ws_client_native.h>

using namespace sanctify;
using namespace indigo;
using namespace core;

namespace {
const char* kLogLabel = "WsClientNative";
}

WsClientNative::WsClientNative(
    std::shared_ptr<indigo::core::TaskList> message_parse_task_list,
    float healthy_time)
    : WsClient(message_parse_task_list, healthy_time) {}

std::shared_ptr<Promise<bool>> WsClientNative::inner_connect(std::string url) {
  auto that = shared_from_this();

  auto rsl_promise = Promise<bool>::create();

  ws_.onOpen([that, rsl_promise]() { that->on_connect(); });
  ws_.onClosed([that]() { that->on_disconnect(); });
  ws_.onError([that](std::string err) { that->on_error(err); });
  ws_.onMessage(
      [that](rtc::message_variant data) {
        if (std::holds_alternative<rtc::string>(data)) {
          // This is a weird and unexpected case! But pass along the data
          // anyways...
          that->recv_raw_msg(std::get<rtc::string>(std::move(data)));
          return;
        }

        if (std::holds_alternative<rtc::binary>(data)) {
          // This is the more expected case - unfortunately it does involve a
          // data copy, but there really isn't any avoiding that.
          const rtc::binary& vdata = std::get<rtc::binary>(data);
          std::string raw_data(vdata.size(), '\0');
          memcpy(&raw_data[0], &vdata[0], vdata.size());
          that->recv_raw_msg(std::move(raw_data));
          return;
        }

        Logger::err(kLogLabel)
            << "WS message was neither string nor binary - which is nonsense!";
      });

  ws_.open(url);

  return rsl_promise;
}

void WsClientNative::send_raw_msg(std::string raw_msg) {
  // This is also shitty, we have to convert to binary because this data is
  // binary...
  rtc::binary bin_msg;
  bin_msg.resize(raw_msg.size());
  memcpy(&bin_msg[0], &raw_msg[0], raw_msg.size());
  if (ws_.isOpen()) {
    ws_.send(std::move(bin_msg));
  }
}

void WsClientNative::destroy_connection() {
  ws_.close();
  ::new (&ws_) rtc::WebSocket();
}