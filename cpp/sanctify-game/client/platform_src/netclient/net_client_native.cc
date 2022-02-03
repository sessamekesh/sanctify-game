#include <netclient/net_client.h>
#include <netclient/ws_client_native.h>

using namespace sanctify;
using namespace indigo;
using namespace core;

NetClient::NetClient(std::shared_ptr<TaskList> message_parse_task_list,
                     float healthy_time)
    : NetClient(std::make_shared<WsClientNative>(message_parse_task_list,
                                                 healthy_time)) {}