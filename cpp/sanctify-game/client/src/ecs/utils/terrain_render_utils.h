#ifndef SANCTIFY_GAME_CLIENT_SRC_ECS_UTILS_TERRAIN_RENDER_UTILS_H
#define SANCTIFY_GAME_CLIENT_SRC_ECS_UTILS_TERRAIN_RENDER_UTILS_H

#include <igasset/igpack_loader.h>
#include <igasync/promise.h>
#include <igcore/maybe.h>
#include <render/terrain/terrain_pipeline.h>
#include <util/resource_registry.h>
#include <webgpu/webgpu_cpp.h>

#include <entt/entt.hpp>
#include <memory>
#include <string>

namespace sanctify::ecs {

//
// Pipeline creation flow
//

/** Error state when creating a terrain pipeline from source promises */
enum class CreateCtxTerrainRenderableError {
  VsLoadError,
  FsLoadError,
  PipelineBuildError
};

/** Stringification for error messages */
std::string to_string(const CreateCtxTerrainRenderableError& err);

/**
 * Method to build a terrain pipeline, and return a promise with either the
 *  error state or resolve empty if pipeline was successfully inserted
 */
std::shared_ptr<
    indigo::core::Promise<indigo::core::Maybe<CreateCtxTerrainRenderableError>>>
create_ctx_terrain_pipeline(
    entt::registry& world, const wgpu::Device& device,
    indigo::asset::IgpackLoader::ExtractWgslShaderPromiseT vs_src_promise,
    indigo::asset::IgpackLoader::ExtractWgslShaderPromiseT fs_src_promise,
    std::shared_ptr<indigo::core::TaskList> main_thread_task_list);

/**
 * Get the current terrain pipeline, re-creating it if the swap chain format has
 *  changed (which under the initial version of WebGPU, it should never do)
 */
terrain_pipeline::TerrainPipeline& get_terrain_pipeline(
    entt::registry& world, const wgpu::Device& device,
    wgpu::TextureFormat swap_chain_format);

/**
 * Render all terrain renderables to the provided render helper. Render helper
 *  is assumed to have been properly initialized with a render pass, as well as
 *  had the correct scene and frame parameters (camera + light data) set.
 */
void render_all_terrain_renderables(terrain_pipeline::RenderUtil& render_util,
                                    entt::registry& world,
                                    const wgpu::Device& device);

/** Error state for loading terrain geo into (context) geo registry */
enum class LoadTerrainGeoError {
  AssetLoadError,
  AssetExtractError,
};
std::string to_string(const LoadTerrainGeoError&);

std::shared_ptr<indigo::core::Promise<indigo::core::Either<
    ReadonlyResourceRegistry<terrain_pipeline::TerrainGeo>::Key,
    LoadTerrainGeoError>>>
load_terrain_geo(
    entt::registry& world, const wgpu::Device& device,
    indigo::asset::IgpackLoader::ExtractDracoBufferPromiseT geo_promise,
    std::shared_ptr<indigo::core::TaskList> main_thread_task_list);

}  // namespace sanctify::ecs

#endif
