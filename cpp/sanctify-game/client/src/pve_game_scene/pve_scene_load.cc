#include "pve_scene_load.h"

#include <ecs/components/common_render_components.h>
#include <ecs/components/terrain_render_components.h>
#include <ecs/utils/terrain_render_utils.h>
#include <igasset/igpack_loader.h>
#include <igasync/promise_combiner.h>
#include <pve_game_scene/ecs/camera.h>
#include <pve_game_scene/ecs/utils.h>
#include <pve_game_scene/io/glfw_io_system.h>
#include <pve_game_scene/render/common_resources.h>
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

// TODO (sessamekesh): Replace this with a Lua definition file
void add_terrain_objects(entt::registry& world) {
  auto& terrain_geo_resources = world.ctx<CtxTerrainBaseGeoResources>();

  // TODO (sessamekesh): Keep track of this entity on a context var somewhere
  auto terrain_base = world.create();
  world.emplace<ecs::TerrainRenderableComponent>(
      terrain_base, terrain_geo_resources.pveArenaGeoKey,
      terrain_geo_resources.pveArenaMaterialKey);
  world.emplace<ecs::MatWorldComponent>(terrain_base, glm::mat4(1.f));
}

void attach_io(entt::registry& world, std::shared_ptr<AppBase> app_base) {
  GlfwIoSystem::attach_glfw_io(world, app_base);
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
  // Synchronous work (setup sync resources). Do this on main thread promises to
  //  break up the work. Only do async work if the written data is known to
  //  never be read on another thread AND there's a provable benefit to going to
  //  a side thread! For example, parsing image data before setting a resource
  //  on the main thread.
  //
  auto main_thread_work = Promise<EmptyPromiseRsl>::schedule(
      main_thread_task_list, [&world, app_base]() -> EmptyPromiseRsl {
        pve::setup_common_3d_gpu_buffers(world, app_base->Device);
        ::attach_io(world, app_base);

        return EmptyPromiseRsl{};
      });

  //
  // Assemble combined promise...
  //
  auto combiner = PromiseCombiner::Create();
  auto terrain_pipeline_key =
      combiner->add(ctx_pipeline_promise, async_task_list);
  auto terrain_base_geo_key =
      combiner->add(terrain_base_geo_resources_promise, async_task_list);
  combiner->add(main_thread_work, main_thread_task_list);

  return combiner->combine()->then<bool>(
      [terrain_pipeline_key, terrain_base_geo_key, &world,
       app_base](const PromiseCombiner::PromiseCombinerResult& rsl) -> bool {
        bool has_error = false;

        ::load_check(rsl, terrain_pipeline_key, has_error, "terrain_pipeline");
        ::load_check(rsl, terrain_base_geo_key, has_error, "terrain_base_geo");

        // If an upstream dependency has failed entirely, don't even bother
        // trying to do any of the creation stuff.
        if (has_error) {
          return false;
        }

        pve::create_terrain_common_bind_groups(
            world, app_base->Device,
            app_base->preferred_swap_chain_texture_format());
        ::add_terrain_objects(world);

        // Success!
        return true;
      },
      async_task_list);
}