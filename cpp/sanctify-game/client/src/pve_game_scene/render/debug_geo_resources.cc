#include "debug_geo_resources.h"

#include <ecs/components/debug_geo_render_components.h>
#include <ecs/utils/debug_geo_render_utils.h>
#include <pve_game_scene/render/common_resources.h>

using namespace sanctify;
using namespace pve;

void DebugGeoResourceUtil::initialize_debug_geo_ctx(
    entt::registry& world, const wgpu::Device& device) {
  auto& pipeline = ecs::DebugGeoRenderUtil::get_pipeline(world);
  auto& common_resources = world.ctx<pve::Common3dGpuBuffers>();

  world.set<CtxDebugGeoBindGroups>(
      pipeline.create_scene_inputs(device, common_resources.commonLightingUbo),
      pipeline.create_frame_inputs(device, common_resources.cameraCommonVsUbo,
                                   common_resources.cameraCommonFsUbo));
}

void DebugGeoResourceUtil::initialize_debug_geo_geo_resources(
    entt::registry& world, const wgpu::Device& device) {
  world.set<CtxDebugGeoGeoResources>(
      ecs::DebugGeoRenderUtil::register_unit_cube(world, device));
}
