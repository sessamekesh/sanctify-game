#ifndef SANCTIFY_PVE_OFFLINE_CLIENT_GAME_SCENE_SYSTEMS_PROCESS_USER_INPUT_H
#define SANCTIFY_PVE_OFFLINE_CLIENT_GAME_SCENE_SYSTEMS_PROCESS_USER_INPUT_H

#include <igecs/world_view.h>

namespace sanctify::pve {

class ProcessUserInputSystem {
 public:
  class EcsUtil {
   public:
  };

 public:
  static const indigo::igecs::WorldView::Decl& decl();
  static void update(indigo::igecs::WorldView* wv);
};

}  // namespace sanctify::pve

#endif
