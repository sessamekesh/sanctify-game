#include <ecs/components/terrain_render_components.h>
#include <ecs/utils/terrain_render_utils.h>
#include <igasync/promise_combiner.h>
#include <pve_game_scene/render/terrain_resources.h>

using namespace sanctify;
using namespace pve;
using namespace indigo;
using namespace core;

std::shared_ptr<Promise<Maybe<LoadCtxTerrainBaseGeoResourceError>>>
pve::load_ctx_terrain_base_geo_resources(
    const wgpu::Device& device, entt::registry& world,
    wgpu::TextureFormat swap_chain_format,
    asset::IgpackLoader::ExtractDracoBufferPromiseT pve_arena_promise,
    std::shared_ptr<TaskList> main_thread_task_list) {
  auto combiner = PromiseCombiner::Create();

  auto arena_base_key =
      combiner->add(ecs::load_terrain_geo(world, device, pve_arena_promise,
                                          main_thread_task_list),
                    main_thread_task_list);

  return combiner->combine()->then<Maybe<LoadCtxTerrainBaseGeoResourceError>>(
      [arena_base_key, &world, &device,
       swap_chain_format](const PromiseCombiner::PromiseCombinerResult& rsl)
          -> Maybe<LoadCtxTerrainBaseGeoResourceError> {
        const auto& arena_base_geo_key_rsl = rsl.get(arena_base_key);
        if (arena_base_geo_key_rsl.is_right()) {
          return LoadCtxTerrainBaseGeoResourceError::ArenaGeoLoadError;
        }

        auto& pipeline =
            ecs::get_terrain_pipeline(world, device, swap_chain_format);

        auto& res = world.ctx_or_set<ecs::CtxTerrainRenderableResources>();
        auto mat_key =
            res.materialRegistry.add_resource(pipeline.create_material_inputs(
                device, glm::vec3{0.2f, 0.2f, 0.2f}));

        world.set<CtxTerrainBaseGeoResources>(arena_base_geo_key_rsl.get_left(),
                                              mat_key);

        return empty_maybe{};
      },
      main_thread_task_list);
}
