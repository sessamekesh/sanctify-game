#ifndef SANCTIFY_PVE_OFFLINE_CLIENT_GAME_SCENE_SYSTEMS_STAGE_CLIENT_MESSAGE_H
#define SANCTIFY_PVE_OFFLINE_CLIENT_GAME_SCENE_SYSTEMS_STAGE_CLIENT_MESSAGE_H

#include <igecs/world_view.h>

namespace sanctify::pve {

// TODO (sessamekesh); Use this to build a message that should be sent to the
//  server using all that crazy logic in net_logic_common. There should be
//  another system that handles transferring client messages to the server.
// Really, this and process_user_input should be in common - not in here!

class StageClientMessageSystem {
 public:
  static const indigo::igecs::WorldView::Decl& decl();
  static void run(indigo::igecs::WorldView* wv);
};

}  // namespace sanctify::pve

#endif
