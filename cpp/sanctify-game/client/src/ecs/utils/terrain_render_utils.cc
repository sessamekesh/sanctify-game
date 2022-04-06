#include <ecs/components/common_render_components.h>
#include <ecs/components/terrain_render_components.h>
#include <ecs/utils/terrain_render_utils.h>
#include <igasync/promise_combiner.h>

using namespace sanctify;
using namespace ecs;
using namespace indigo;
using namespace core;

std::string ecs::to_string(const CreateCtxTerrainRenderableError& err) {
  switch (err) {
    case CreateCtxTerrainRenderableError::FsLoadError:
      return "CreateCtxTerrainRenderableError::FsLoadError";
    case CreateCtxTerrainRenderableError::VsLoadError:
      return "CreateCtxTerrainRenderableError::VsLoadError";
    case CreateCtxTerrainRenderableError::PipelineBuildError:
      return "CreateCtxTerrainRenderableError::PipelineBuildError";
    default:
      return "CreateCtxTerrainRenderableError::Unknown";
  }
}

std::shared_ptr<Promise<Maybe<CreateCtxTerrainRenderableError>>>
ecs::create_ctx_terrain_pipeline(
    entt::registry& world, const wgpu::Device& device,
    asset::IgpackLoader::ExtractWgslShaderPromiseT vs_src_promise,
    asset::IgpackLoader::ExtractWgslShaderPromiseT fs_src_promise,
    std::shared_ptr<indigo::core::TaskList> main_thread_task_list) {
  auto combiner = PromiseCombiner::Create();

  auto vs_key = combiner->add(vs_src_promise, main_thread_task_list);
  auto fs_key = combiner->add(fs_src_promise, main_thread_task_list);

  return combiner->combine()->then<Maybe<CreateCtxTerrainRenderableError>>(
      [vs_key, fs_key, device,
       &world](const PromiseCombiner::PromiseCombinerResult& rsl)
          -> Maybe<CreateCtxTerrainRenderableError> {
        const auto& vs_rsl = rsl.get(vs_key);
        const auto& fs_rsl = rsl.get(fs_key);

        if (vs_rsl.is_right()) {
          Logger::err("create_ctx_terrain_renderable_resources")
              << "Failed to load vertex shader: "
              << to_string(vs_rsl.get_right());
          return CreateCtxTerrainRenderableError::VsLoadError;
        }

        if (fs_rsl.is_right()) {
          Logger::err("create_ctx_terrain_renderable_resources")
              << "Failed to load fragment shader: "
              << to_string(fs_rsl.get_right());
          return CreateCtxTerrainRenderableError::FsLoadError;
        }

        auto pipeline_builder =
            terrain_pipeline::TerrainPipelineBuilder::Create(
                device, vs_rsl.get_left(), fs_rsl.get_left());

        if (pipeline_builder.is_empty()) {
          return CreateCtxTerrainRenderableError::PipelineBuildError;
        }

        world.set<CtxTerrainPipeline>(pipeline_builder.move(), empty_maybe{});
        return empty_maybe{};
      },
      main_thread_task_list);
}

terrain_pipeline::TerrainPipeline& ecs::get_terrain_pipeline(
    entt::registry& world, const wgpu::Device& device,
    wgpu::TextureFormat swap_chain_format) {
  assert(world.try_ctx<CtxTerrainPipeline>() != nullptr);

  auto& ctx_terrain_pipeline = world.ctx<CtxTerrainPipeline>();

  if (ctx_terrain_pipeline.pipeline.is_empty() ||
      ctx_terrain_pipeline.pipeline.get().OutputFormat != swap_chain_format) {
    ctx_terrain_pipeline.pipeline =
        ctx_terrain_pipeline.pipelineBuilder.create_pipeline(device,
                                                             swap_chain_format);
  }

  return ctx_terrain_pipeline.pipeline.get();
}

// TODO (sessamekesh): Add in a dirtying mechanism to prevent re-calculating
//  instance buffers for static geometry? (Might not be worth it for frustum
//  culling?)
void ecs::render_all_terrain_renderables(
    terrain_pipeline::RenderUtil& render_util, entt::registry& world,
    const wgpu::Device& device) {
  auto view =
      world.view<const TerrainRenderableComponent, const MatWorldComponent>();

  // TODO (sessamekesh): Put this into a pool to avoid allocations?
  struct ResourceKeyPair {
    ReadonlyResourceRegistry<terrain_pipeline::TerrainGeo>::Key geoKey;
    ReadonlyResourceRegistry<terrain_pipeline::MaterialPipelineInputs>::Key
        materialKey;
  };
  core::Vector<std::string> keys(2);
  core::Vector<core::PodVector<terrain_pipeline::TerrainMatWorldInstanceData>>
      values(2);
  core::Vector<ResourceKeyPair> key_pairs(2);
  for (auto [e, renderable_component, mat_world] : view.each()) {
    auto key = terrain_pipeline::TerrainInstanceBufferStore::instancing_key(
        renderable_component.geoKey, renderable_component.materialKey);

    int key_idx = -1;
    for (int i = 0; i < keys.size(); i++) {
      if (keys[i] == key) {
        key_idx = i;
        break;
      }
    }

    if (key_idx >= 0) {
      values[key_idx].push_back(
          terrain_pipeline::TerrainMatWorldInstanceData{mat_world.matWorld});
    } else {
      keys.push_back(key);
      values.push_back(
          PodVector<terrain_pipeline::TerrainMatWorldInstanceData>(2));
      key_pairs.push_back(
          {renderable_component.geoKey, renderable_component.materialKey});
      values.last().push_back(
          terrain_pipeline::TerrainMatWorldInstanceData{mat_world.matWorld});
    }
  }

  auto& registries = world.ctx_or_set<CtxTerrainRenderableResources>();
  for (int i = 0; i < keys.size(); i++) {
    auto& instance_buffer = registries.instanceBufferStore.get_instance_buffer(
        device, key_pairs[i].geoKey, key_pairs[i].materialKey, values[i], 5);
    const auto* geo = registries.geoRegistry.get(key_pairs[i].geoKey);
    const auto* mat = registries.materialRegistry.get(key_pairs[i].materialKey);

    if (!geo || !mat) {
      continue;
    }

    render_util.set_geometry(*geo)
        .set_material_inputs(*mat)
        .set_instances(instance_buffer, values[i].size())
        .draw();
  }
}

std::shared_ptr<
    Promise<Either<ReadonlyResourceRegistry<terrain_pipeline::TerrainGeo>::Key,
                   LoadTerrainGeoError>>>
ecs::load_terrain_geo(
    entt::registry& world, const wgpu::Device& device,
    indigo::asset::IgpackLoader::ExtractDracoBufferPromiseT geo_promise,
    std::shared_ptr<TaskList> main_thread_task_list) {
  return geo_promise->then<
      Either<ReadonlyResourceRegistry<terrain_pipeline::TerrainGeo>::Key,
             LoadTerrainGeoError>>(
      [&device, &world](const asset::IgpackLoader::ExtractDracoBufferT& rsl)
          -> Either<ReadonlyResourceRegistry<terrain_pipeline::TerrainGeo>::Key,
                    LoadTerrainGeoError> {
        if (rsl.is_right()) {
          Logger::err("ecs::load_terrain_geo")
              << "Asset load error: " << asset::to_string(rsl.get_right());
          return right(LoadTerrainGeoError::AssetLoadError);
        }

        const auto& decoder = rsl.get_left();

        auto pos_norm_rsl = decoder->get_pos_norm_data();
        auto indices_rsl = decoder->get_index_data();

        if (pos_norm_rsl.is_right()) {
          Logger::err("ecs::load_terrain_geo")
              << "Pos-norm vertex buffer extract error: "
              << asset::to_string(pos_norm_rsl.get_right());
          return right(LoadTerrainGeoError::AssetExtractError);
        }
        if (indices_rsl.is_right()) {
          Logger::err("ecs::load_terrain_geo")
              << "Index buffer extract error: "
              << asset::to_string(pos_norm_rsl.get_right());
          return right(LoadTerrainGeoError::AssetExtractError);
        }

        auto& reg = world.ctx_or_set<CtxTerrainRenderableResources>();
        return left(reg.geoRegistry.add_resource(terrain_pipeline::TerrainGeo(
            device, pos_norm_rsl.get_left(), indices_rsl.get_left())));
      },
      main_thread_task_list);
}
