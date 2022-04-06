#ifndef SANCTIFY_GAME_CLIENT_PVE_GAME_SCENE_RENDER_TERRAIN_RESOURCES_H
#define SANCTIFY_GAME_CLIENT_PVE_GAME_SCENE_RENDER_TERRAIN_RESOURCES_H

#include <igasset/igpack_loader.h>
#include <render/terrain/terrain_geo.h>
#include <render/terrain/terrain_pipeline.h>
#include <util/resource_registry.h>

namespace sanctify::pve {

//
// Game specific arena stuff! Context state for all terrain renderables
//

struct CtxTerrainBaseGeoResources {
  ReadonlyResourceRegistry<terrain_pipeline::TerrainGeo>::Key pveArenaGeoKey;
  ReadonlyResourceRegistry<terrain_pipeline::MaterialPipelineInputs>::Key
      pveArenaMaterialKey;
};

enum class LoadCtxTerrainBaseGeoResourceError {
  ArenaGeoLoadError,
  PipelineMissingError,
};

std::shared_ptr<indigo::core::Promise<
    indigo::core::Maybe<LoadCtxTerrainBaseGeoResourceError>>>
load_ctx_terrain_base_geo_resources(
    const wgpu::Device& device, entt::registry& world,
    wgpu::TextureFormat swap_chain_format,
    indigo::asset::IgpackLoader::ExtractDracoBufferPromiseT pve_arena_promise,
    std::shared_ptr<indigo::core::TaskList> main_thread_task_list);

}  // namespace sanctify::pve

#endif
