#include <ecs/components/terrain_render_components.h>
#include <pve_game_scene/render/common_resources.h>
#include <pve_game_scene/render/loading_screen.h>

using namespace sanctify::pve;

namespace {
wgpu::RenderPassEncoder create_main_pass(
    const wgpu::CommandEncoder& encoder,
    const FinalRenderTargetBuffers& targets) {
  wgpu::RenderPassDepthStencilAttachment depth_attachment{};
  depth_attachment.clearDepth = 1.f;
  depth_attachment.clearStencil = 0x00;
  depth_attachment.depthLoadOp = wgpu::LoadOp::Clear;
  depth_attachment.depthStoreOp = wgpu::StoreOp::Store;
  depth_attachment.stencilLoadOp = wgpu::LoadOp::Clear;
  depth_attachment.stencilStoreOp = wgpu::StoreOp::Discard;
  depth_attachment.view = targets.depthStencilView;

  wgpu::RenderPassColorAttachment color_attachment{};
  color_attachment.clearColor = {0.1f, 0.1f, 0.17f, 1.f};
  color_attachment.loadOp = wgpu::LoadOp::Clear;
  color_attachment.storeOp = wgpu::StoreOp::Store;
  color_attachment.view = targets.colorTarget;

  wgpu::RenderPassDescriptor desc{};
  desc.colorAttachmentCount = 1;
  desc.colorAttachments = &color_attachment;
  desc.depthStencilAttachment = &depth_attachment;

  return encoder.BeginRenderPass(&desc);
}
}  // namespace

void LoadingScreenRenderSystem::render(const wgpu::Device& device,
                                       entt::registry& world) {
  //
  // MAIN PASS
  //
  auto& final_target_buffers = world.ctx<FinalRenderTargetBuffers>();

  wgpu::CommandEncoder main_pass_command_encoder =
      device.CreateCommandEncoder();
  wgpu::RenderPassEncoder main_pass =
      ::create_main_pass(main_pass_command_encoder, final_target_buffers);

  main_pass.End();

  wgpu::CommandBuffer main_pass_commands = main_pass_command_encoder.Finish();

  device.GetQueue().Submit(1, &main_pass_commands);

  //
  // CLEANUP RENDER RESOURCES
  //
  world.ctx_or_set<ecs::CtxTerrainRenderableResources>()
      .instanceBufferStore.mark_frame_and_cleanup_dead_buffers();
}
