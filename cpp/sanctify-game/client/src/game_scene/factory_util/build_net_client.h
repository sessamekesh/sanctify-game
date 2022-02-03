#ifndef SANCTIFY_GAME_CLIENT_SRC_GAME_SCENE_FACTORY_UTIL_BULID_NET_CLIENT_H
#define SANCTIFY_GAME_CLIENT_SRC_GAME_SCENE_FACTORY_UTIL_BULID_NET_CLIENT_H

#include <igasync/promise.h>
#include <igcore/maybe.h>
#include <netclient/net_client.h>

namespace sanctify {

std::shared_ptr<
    indigo::core::Promise<indigo::core::Maybe<std::shared_ptr<NetClient>>>>
build_net_client(std::string api_url_base,
                 std::shared_ptr<indigo::core::TaskList> async_task_list);

}

#endif
