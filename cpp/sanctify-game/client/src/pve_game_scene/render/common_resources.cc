#include <pve_game_scene/render/common_resources.h>

using namespace sanctify;

void pve::set_frame_render_targets(entt::registry& world,
                                   wgpu::TextureView color_target,
                                   wgpu::TextureFormat color_target_format,
                                   wgpu::TextureView depth_stencil_view) {
  auto& component = world.ctx_or_set<pve::FinalRenderTargetBuffers>();

  component.colorTargetFormat = color_target_format;
  component.colorTarget = color_target;
  component.depthStencilView = depth_stencil_view;
}
