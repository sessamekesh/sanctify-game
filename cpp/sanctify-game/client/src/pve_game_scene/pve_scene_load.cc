#include "pve_scene_load.h"

#include <ecs/utils/terrain_render_utils.h>
#include <igasset/igpack_loader.h>
#include <igasync/promise_combiner.h>
#include <pve_game_scene/ecs/utils.h>
#include <pve_game_scene/render/terrain_resources.h>

using namespace sanctify;
using namespace pve;
using namespace indigo;
using namespace core;

using std::bind;
using std::placeholders::_1;

namespace {
const char* kLogLabel = "PveSceneLoader";

template <typename T>
void load_check(const PromiseCombiner::PromiseCombinerResult& rsl,
                PromiseCombiner::PromiseCombinerKey<Maybe<T>> key,
                bool& err_flag, std::string help_name) {
  rsl.get(key).if_present([&err_flag, help_name](const auto&) {
    Logger::err(kLogLabel) << "Load check failed for '" << help_name << "'";
    err_flag = true;
  });
}

}  // namespace

std::shared_ptr<Promise<bool>> pve::load_pve_scene(
    entt::registry& world, std::shared_ptr<TaskList> main_thread_task_list,
    std::shared_ptr<TaskList> async_task_list,
    std::shared_ptr<AppBase> app_base) {
  //
  // Asset loaders...
  //
  indigo::asset::IgpackLoader terrain_igpack_loader(
      "resources/pve-terrain-geo.igpack", async_task_list);
  indigo::asset::IgpackLoader terrain_shaders_loader(
      "resources/terrain-shaders.igpack", async_task_list);

  //
  // Individual asset promises...
  //
  auto terrain_vs_promise = terrain_shaders_loader.extract_wgsl_shader(
      "solidVertWgsl", async_task_list);
  auto terrain_fs_promise = terrain_shaders_loader.extract_wgsl_shader(
      "solidFragWgsl", async_task_list);
  auto terrain_base_geo_promise = terrain_igpack_loader.extract_draco_geo(
      "terrainBaseGeo", async_task_list);

  //
  // Run all the preliminary setup actions
  //
  auto ctx_pipeline_promise = ecs::create_ctx_terrain_pipeline(
      world, app_base->Device, terrain_vs_promise, terrain_fs_promise,
      main_thread_task_list);
  auto terrain_base_geo_resources_promise = ctx_pipeline_promise->then_chain<
      Maybe<LoadCtxTerrainBaseGeoResourceError>>(
      [app_base, &world, terrain_base_geo_promise,
       main_thread_task_list](const auto& rsl) {
        if (rsl.has_value()) {
          return Promise<Maybe<LoadCtxTerrainBaseGeoResourceError>>::immediate(
              LoadCtxTerrainBaseGeoResourceError::PipelineMissingError);
        }

        return pve::load_ctx_terrain_base_geo_resources(
            app_base->Device, world,
            app_base->preferred_swap_chain_texture_format(),
            terrain_base_geo_promise, main_thread_task_list);
      },
      async_task_list);

  //
  // Assemble combined promise...
  //
  auto combiner = PromiseCombiner::Create();
  auto terrain_pipeline_key =
      combiner->add(ctx_pipeline_promise, async_task_list);
  auto terrain_base_geo_key =
      combiner->add(terrain_base_geo_resources_promise, async_task_list);

  return combiner->combine()->then<bool>(
      [terrain_pipeline_key, terrain_base_geo_key](
          const PromiseCombiner::PromiseCombinerResult& rsl) -> bool {
        bool has_error = false;

        ::load_check(rsl, terrain_pipeline_key, has_error, "terrain_pipeline");
        ::load_check(rsl, terrain_base_geo_key, has_error, "terrain_base_geo");

        // If an upstream dependency has failed entirely, don't even bother
        // trying to do any of the creation stuff.
        if (has_error) {
          return false;
        }

        // Success!
        return true;
      },
      async_task_list);
}