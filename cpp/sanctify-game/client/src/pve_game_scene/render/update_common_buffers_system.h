#ifndef SANCTIFY_GAME_CLIENT_SRC_PVE_GAME_SCENE_RENDER_UPDATE_COMMON_BUFFERS_SYSTEM_H
#define SANCTIFY_GAME_CLIENT_SRC_PVE_GAME_SCENE_RENDER_UPDATE_COMMON_BUFFERS_SYSTEM_H

#include <webgpu/webgpu_cpp.h>

#include <entt/entt.hpp>

namespace sanctify::pve {

class UpdateCommonBuffersSystem {
 public:
  static void update(entt::registry& world, const wgpu::Device& device);
};

}  // namespace sanctify::pve

#endif
