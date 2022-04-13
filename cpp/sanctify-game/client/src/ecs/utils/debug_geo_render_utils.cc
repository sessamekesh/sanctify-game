#include "debug_geo_render_utils.h"

#include <ecs/components/common_render_components.h>
#include <igasync/promise_combiner.h>

using namespace sanctify;
using namespace ecs;
using namespace indigo;
using namespace core;

namespace {
const char* kLogLabel = "DebugGeoRenderUtil";
}

std::shared_ptr<Promise<Maybe<DebugGeoRenderUtil::LoadError>>>
DebugGeoRenderUtil::init_pipeline_builder(
    entt::registry& world, const wgpu::Device& device,
    asset::IgpackLoader::ExtractWgslShaderPromiseT vs_src_promise,
    asset::IgpackLoader::ExtractWgslShaderPromiseT fs_src_promise,
    std::shared_ptr<TaskList> main_thread_task_list) {
  auto combiner = PromiseCombiner::Create();

  auto vs_key = combiner->add(vs_src_promise, main_thread_task_list);
  auto fs_key = combiner->add(fs_src_promise, main_thread_task_list);

  return combiner->combine()->then<Maybe<LoadError>>(
      [vs_key, fs_key, &world,
       device](const PromiseCombiner::PromiseCombinerResult& rsl)
          -> Maybe<LoadError> {
        const auto& vs_src_rsl = rsl.get(vs_key);
        const auto& fs_src_rsl = rsl.get(fs_key);

        if (vs_src_rsl.is_right()) {
          Logger::err(kLogLabel) << "Could not load VS from source";
          return LoadError::VsSrcNotFound;
        }

        if (fs_src_rsl.is_right()) {
          Logger::err(kLogLabel) << "Could not load FS from source";
          return LoadError::FsSrcNotFound;
        }

        Maybe<debug_geo::DebugGeoPipelineBuilder> maybe_pipeline_builder =
            debug_geo::DebugGeoPipelineBuilder::Create(
                device, vs_src_rsl.get_left(), fs_src_rsl.get_left());

        if (maybe_pipeline_builder.is_empty()) {
          Logger::err(kLogLabel)
              << "Failed to build the debug geo pipeline builder";
          return LoadError::BuildFail;
        }

        world.set<CtxDebugGeoPipelineBuilder>(maybe_pipeline_builder.move());

        return empty_maybe{};
      },
      main_thread_task_list);
}

void DebugGeoRenderUtil::init_pipeline(entt::registry& world,
                                       const wgpu::Device& device,
                                       wgpu::TextureFormat swap_chain_format) {
  auto& builder = world.ctx<CtxDebugGeoPipelineBuilder>().pipelineBuilder;
  auto* ctx_pipeline = world.try_ctx<CtxDebugGeoPipeline>();

  // Lazy set pipeline
  if (!ctx_pipeline) {
    world.set<CtxDebugGeoPipeline>(
        builder.create_pipeline(device, swap_chain_format));
  } else if (ctx_pipeline->swapChainFormat != swap_chain_format) {
    ctx_pipeline->pipeline = builder.create_pipeline(device, swap_chain_format);
    ctx_pipeline->swapChainFormat = swap_chain_format;
  }
}

debug_geo::DebugGeoPipeline& DebugGeoRenderUtil::get_pipeline(
    entt::registry& world) {
  return world.ctx<CtxDebugGeoPipeline>().pipeline;
}

void DebugGeoRenderUtil::render_all_debug_geo_renderables(
    debug_geo::RenderUtil& render_util, entt::registry& world,
    const wgpu::Device& device) {
  auto& renderable_resources =
      world.ctx_or_set<CtxDebugGeoRenderableResources>();

  auto view =
      world.view<const DebugGeoRenderable, const ecs::MatWorldComponent>();

  renderable_resources.instanceBufferStore.begin_frame();

  for (auto [e, debug_geo, mat_world] : view.each()) {
    debug_geo::InstanceKey key{debug_geo.geoKey, 4};
    debug_geo::InstanceData data{};
    data.MatWorld = mat_world.matWorld;
    data.ObjectColor = debug_geo.objectColor;
    renderable_resources.instanceBufferStore.add_instance(key, data);
  }

  renderable_resources.instanceBufferStore.finalize(
      device, [&renderable_resources, &render_util](
                  const debug_geo::InstanceKey& key, const wgpu::Buffer& ib,
                  uint32_t num_instances) {
        auto* geo = renderable_resources.geoRegistry.get(key.geoKey);
        if (!geo) return;

        render_util.set_geometry(*geo).set_instances(ib, num_instances).draw();
      });
}

ReadonlyResourceRegistry<debug_geo::DebugGeo>::Key
DebugGeoRenderUtil::register_unit_cube(entt::registry& world,
                                       const wgpu::Device& device) {
  auto& registries = world.ctx_or_set<CtxDebugGeoRenderableResources>();

  return registries.geoRegistry.add_resource(
      debug_geo::DebugGeo::CreateDebugUnitCube(device));
}

std::string ecs::to_string(const DebugGeoRenderUtil::LoadError& err) {
  switch (err) {
    case DebugGeoRenderUtil::LoadError::VsSrcNotFound:
      return "dgru::VsSrcNotFound";
    case DebugGeoRenderUtil::LoadError::FsSrcNotFound:
      return "dgru::FsSrcNotFound";
    case DebugGeoRenderUtil::LoadError::BuildFail:
      return "dgru::BuildFail";
  }
}
