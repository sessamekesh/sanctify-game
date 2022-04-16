#include "render_game.h"

#include <ecs/components/terrain_render_components.h>
#include <ecs/utils/debug_geo_render_utils.h>
#include <ecs/utils/terrain_render_utils.h>
#include <pve_game_scene/render/common_resources.h>
#include <pve_game_scene/render/debug_geo_resources.h>
#include <pve_game_scene/render/terrain_resources.h>
#include <pve_game_scene/render/update_common_buffers_system.h>
#include <render/terrain/terrain_pipeline.h>

using namespace sanctify;
using namespace pve;

namespace {
const char* kLogLabel = "PveGameRenderSystem";

wgpu::RenderPassEncoder create_main_pass(
    const wgpu::CommandEncoder& encoder,
    const FinalRenderTargetBuffers& targets) {
  wgpu::RenderPassDepthStencilAttachment depth_attachment{};
  depth_attachment.depthClearValue = 1.f;
  depth_attachment.stencilClearValue = 0x00;
  depth_attachment.depthLoadOp = wgpu::LoadOp::Clear;
  depth_attachment.depthStoreOp = wgpu::StoreOp::Store;
  depth_attachment.stencilLoadOp = wgpu::LoadOp::Clear;
  depth_attachment.stencilStoreOp = wgpu::StoreOp::Discard;
  depth_attachment.view = targets.depthStencilView;

  wgpu::RenderPassColorAttachment color_attachment{};
  color_attachment.clearValue = {0.1f, 0.1f, 0.17f, 1.f};
  color_attachment.loadOp = wgpu::LoadOp::Clear;
  color_attachment.storeOp = wgpu::StoreOp::Store;
  color_attachment.view = targets.colorTarget;

  wgpu::RenderPassDescriptor desc{};
  desc.colorAttachmentCount = 1;
  desc.colorAttachments = &color_attachment;
  desc.depthStencilAttachment = &depth_attachment;

  return encoder.BeginRenderPass(&desc);
}

struct TerrainFrameAndSceneResources {
  terrain_pipeline::ScenePipelineInputs sceneInputs;
  terrain_pipeline::FramePipelineInputs frameInputs;
};

void set_terrain_resources(terrain_pipeline::RenderUtil& render_util,
                           const wgpu::Device& device, entt::registry& world,
                           wgpu::TextureFormat swap_chain_format) {
  if (!world.try_ctx<TerrainFrameAndSceneResources>()) {
    auto& pipeline =
        ecs::get_terrain_pipeline(world, device, swap_chain_format);

    world.set<TerrainFrameAndSceneResources>();
  }
}

}  // namespace

void GameRenderSystem::render(const wgpu::Device& device, entt::registry& world,
                              wgpu::TextureFormat swap_chain_format) {
  //
  // MAIN PASS
  //
  auto& final_target_buffers = world.ctx<FinalRenderTargetBuffers>();

  wgpu::CommandEncoder main_pass_command_encoder =
      device.CreateCommandEncoder();
  wgpu::RenderPassEncoder main_pass =
      ::create_main_pass(main_pass_command_encoder, final_target_buffers);

  // Get common things (camera...)
  UpdateCommonBuffersSystem::update(world, device);

  // Terrain geometry...
  {
    auto& terrain_pipeline =
        ecs::get_terrain_pipeline(world, device, swap_chain_format);
    auto& bind_groups = world.ctx<CtxTerrainCommonBindGroups>();

    terrain_pipeline::RenderUtil util(device, main_pass, terrain_pipeline);
    util.set_frame_inputs(bind_groups.frameInputs)
        .set_scene_inputs(bind_groups.sceneInputs);

    ecs::render_all_terrain_renderables(util, world, device);
  }
  // Debug geometry...
  {
    auto& pipeline = ecs::DebugGeoRenderUtil::get_pipeline(world);
    auto& bind_groups = world.ctx<CtxDebugGeoBindGroups>();

    debug_geo::RenderUtil util(main_pass, pipeline);
    util.set_frame_inputs(bind_groups.frameInputs)
        .set_scene_inputs(bind_groups.sceneInputs);

    // TODO (sessamekesh): First attach bind groups!
    ecs::DebugGeoRenderUtil::render_all_debug_geo_renderables(util, world,
                                                              device);
  }
  // TODO (sessamekesh): Create

  main_pass.End();

  wgpu::CommandBuffer main_pass_commands = main_pass_command_encoder.Finish();
  device.GetQueue().Submit(1, &main_pass_commands);

  //
  // CLEANUP RENDER RESOURCES
  //
  world.ctx_or_set<ecs::CtxTerrainRenderableResources>()
      .instanceBufferStore.mark_frame_and_cleanup_dead_buffers();
}
