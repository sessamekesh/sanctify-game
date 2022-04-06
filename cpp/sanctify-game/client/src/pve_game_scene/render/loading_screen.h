#ifndef SANCTIFY_GAME_CLIENT_PVE_GAME_SCENE_RENDER_LOADING_SCREEN_H
#define SANCTIFY_GAME_CLIENT_PVE_GAME_SCENE_RENDER_LOADING_SCREEN_H

#include <webgpu/webgpu_cpp.h>

#include <entt/entt.hpp>

namespace sanctify::pve {

class LoadingScreenRenderSystem {
 public:
  static void render(const wgpu::Device& device, entt::registry& world);
};

}  // namespace sanctify::pve

#endif
