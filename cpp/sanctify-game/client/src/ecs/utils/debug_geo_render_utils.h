#ifndef SANCTIFY_GAME_CLIENT_SRC_ECS_UTILS_DEBUG_GEO_RENDER_UTILS_H
#define SANCTIFY_GAME_CLIENT_SRC_ECS_UTILS_DEBUG_GEO_RENDER_UTILS_H

#include <ecs/components/debug_geo_render_components.h>
#include <igasset/igpack_loader.h>
#include <igasync/promise.h>

#include <entt/entt.hpp>

// TODO (sessamekesh): Add functionality for working with debug geo stuff
// - Pipeline loader method
// - Pipeline refresh method (when swap chain format changes)
// - Debug geo creation methods (generate and upload separate, to take advantage
//   of async!)

namespace sanctify::ecs {

class DebugGeoRenderUtil {
 public:
  enum class LoadError {
    VsSrcNotFound,
    FsSrcNotFound,
    BuildFail,
  };
  static std::shared_ptr<indigo::core::Promise<indigo::core::Maybe<LoadError>>>
  init_pipeline_builder(
      entt::registry& world, const wgpu::Device& device,
      indigo::asset::IgpackLoader::ExtractWgslShaderPromiseT vs_src_promise,
      indigo::asset::IgpackLoader::ExtractWgslShaderPromiseT fs_src_promise,
      std::shared_ptr<indigo::core::TaskList> main_thread_task_list);

  static void init_pipeline(entt::registry& world, const wgpu::Device& device,
                            wgpu::TextureFormat swap_chain_format);

  static debug_geo::DebugGeoPipeline& get_pipeline(entt::registry& world);

  static void render_all_debug_geo_renderables(
      debug_geo::RenderUtil& render_util, entt::registry& world,
      const wgpu::Device& device);

  static ReadonlyResourceRegistry<debug_geo::DebugGeo>::Key register_unit_cube(
      entt::registry& world, const wgpu::Device& device);
};

static std::string to_string(const DebugGeoRenderUtil::LoadError& err);

}  // namespace sanctify::ecs

#endif
