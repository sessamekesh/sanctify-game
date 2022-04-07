#ifndef SANCTIFY_GAME_CLIENT_SRC_PVE_GAME_SCENE_RENDER_RENDER_GAME_H
#define SANCTIFY_GAME_CLIENT_SRC_PVE_GAME_SCENE_RENDER_RENDER_GAME_H

#include <webgpu/webgpu_cpp.h>

#include <entt/entt.hpp>

namespace sanctify::pve {

class GameRenderSystem {
 public:
  static void render(const wgpu::Device& device, entt::registry& world,
                     wgpu::TextureFormat swap_chain_format);
};

// ...

}  // namespace sanctify::pve

#endif
