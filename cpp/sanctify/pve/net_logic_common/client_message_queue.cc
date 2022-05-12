#include "client_message_queue.h"

using namespace sanctify;
using namespace pve;

void ClientMessageQueueUtil::enqueue_client_action(
    indigo::igecs::WorldView* wv, proto::PveClientSingleMessage msg) {
  wv->mut_ctx_or_set<CtxUnsentClientActions>().messages.push_back(msg);
}

bool ClientMessageQueueUtil::get_client_actions(
    indigo::igecs::WorldView* wv, int max_messages,
    proto::PveClientActionsList* o_actions_list) {
  auto& ctx_actions = wv->mut_ctx_or_set<CtxUnsentClientActions>().messages;

  if (ctx_actions.size() == 0) {
    return false;
  }

  int start_idx = 0;
  if (ctx_actions.size() > max_messages) {
    start_idx = ctx_actions.size() - max_messages;
  }

  for (int i = start_idx; i < ctx_actions.size(); i++) {
    *o_actions_list->add_actions() = ctx_actions[i];
  }

  ctx_actions.clear();
  return true;
}
