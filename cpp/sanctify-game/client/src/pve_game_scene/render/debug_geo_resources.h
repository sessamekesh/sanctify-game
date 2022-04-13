#ifndef SANCTIFY_GAME_CLIENT_PVE_GAME_SCENE_RENDER_DEBUG_GEO_RESOURCES_H
#define SANCTIFY_GAME_CLIENT_PVE_GAME_SCENE_RENDER_DEBUG_GEO_RESOURCES_H

#include <render/debug_geo/debug_geo.h>
#include <render/debug_geo/debug_geo_pipeline.h>
#include <util/resource_registry.h>

#include <entt/entt.hpp>

namespace sanctify::pve {

//
// Debug geo keys
//
struct CtxDebugGeoGeoResources {
  ReadonlyResourceRegistry<debug_geo::DebugGeo>::Key unitCubeKey;
};

//
// Frame / scene bind groups
//
struct CtxDebugGeoBindGroups {
  debug_geo::ScenePipelineInputs sceneInputs;
  debug_geo::FramePipelineInputs frameInputs;
};

class DebugGeoResourceUtil {
 public:
  static void initialize_debug_geo_ctx(entt::registry& world,
                                       const wgpu::Device& device);
  static void initialize_debug_geo_geo_resources(entt::registry& world,
                                                 const wgpu::Device& device);
};

}  // namespace sanctify::pve

#endif
