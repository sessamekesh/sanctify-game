#ifndef SANCTIFY_PVE_OFFLINE_CLIENT_GAME_SCENE_PVE_OFFLINE_RENDER_H
#define SANCTIFY_PVE_OFFLINE_CLIENT_GAME_SCENE_PVE_OFFLINE_RENDER_H

#include <igecs/world_view.h>

namespace sanctify::pve {

class PveOfflineRenderSystem {
 public:
  static const indigo::igecs::WorldView::Decl& render_decl();
  static void render(indigo::igecs::WorldView* wv);
};

}  // namespace sanctify::pve

#endif
