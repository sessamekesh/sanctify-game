#ifndef SANCTIFY_PVE_NET_LOGIC_COMMON_H
#define SANCTIFY_PVE_NET_LOGIC_COMMON_H

#include <igcore/vector.h>
#include <igecs/world_view.h>
#include <sanctify/pve/net_logic_common/proto/client_messages.pb.h>

namespace sanctify::pve {

struct CtxUnsentClientActions {
  indigo::core::Vector<proto::PveClientSingleMessage> messages;
};

class ClientMessageQueueUtil {
 public:
  static void enqueue_client_action(indigo::igecs::WorldView* wv,
                                    proto::PveClientSingleMessage msg);
  static bool get_client_actions(indigo::igecs::WorldView* wv, int max_messages,
                                 proto::PveClientActionsList* o_actions_list);
};

}  // namespace sanctify::pve

#endif
