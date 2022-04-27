#include "ecs_util.h"

#include <common/render/common/render_components.h>
#include <igasync/promise_combiner.h>
#include <igcore/log.h>

using namespace sanctify;
using namespace render;
using namespace solid_static;

using namespace indigo;
using namespace core;

namespace {
const char* kLogLabel = "s::r::solid_static::EcsUtil";
}

bool EcsUtil::prepare_world(indigo::igecs::WorldView* wv) {
  if (!wv->ctx_has<CtxHdrFramebufferParams>()) {
    return false;
  }

  wv->attach_ctx<CtxResourceRegistries>();

  return true;
}

void EcsUtil::attach_renderable(igecs::WorldView* wv, entt::entity e,
                                ReadonlyResourceRegistry<Geo>::Key geo_key,
                                InstanceData instance_data) {
  wv->write<MatWorldComponent>(e).matWorld = instance_data.matWorld;
  wv->attach<RenderableComponent>(
      e, geo_key, instance_data.albedo, instance_data.metallic,
      instance_data.roughness, instance_data.ambientOcclusion);
}

void EcsUtil::detach_renderable(igecs::WorldView* wv, entt::entity e) {
  wv->remove<MatWorldComponent>(e);
  wv->remove<RenderableComponent>(e);
}

std::shared_ptr<Promise<Maybe<PipelineBuildError>>>
EcsUtil::create_ctx_pipeline(
    entt::registry* world, const wgpu::Device& device,
    asset::IgpackLoader::ExtractWgslShaderPromiseT vs_src_promise,
    asset::IgpackLoader::ExtractWgslShaderPromiseT fs_src_promise,
    std::shared_ptr<TaskList> main_thread_task_list) {
  auto combiner = PromiseCombiner::Create();

  auto vs_key = combiner->add(vs_src_promise, main_thread_task_list);
  auto fs_key = combiner->add(fs_src_promise, main_thread_task_list);

  return combiner->combine<Maybe<PipelineBuildError>>(
      [vs_key, fs_key, device,
       world](const PromiseCombiner::PromiseCombinerResult& rsl)
          -> Maybe<PipelineBuildError> {
        const auto& vs_rsl = rsl.get(vs_key);
        const auto& fs_rsl = rsl.get(fs_key);

        if (vs_rsl.is_right()) {
          Logger::err(kLogLabel) << "Failed to load vertex shader: "
                                 << asset::to_string(vs_rsl.get_right());
          return PipelineBuildError::VsLoadError;
        }
        if (fs_rsl.is_right()) {
          Logger::err(kLogLabel) << "Failed to load fragment shader: "
                                 << asset::to_string(fs_rsl.get_right());
          return PipelineBuildError::FsLoadError;
        }

        auto pipeline_builder = PipelineBuilder::Create(
            device, vs_rsl.get_left(), fs_rsl.get_left());

        wgpu::TextureFormat hdr_format =
            world->ctx<CtxHdrFramebufferParams>().format;

        auto pipeline = pipeline_builder.create_pipeline(device, hdr_format);

        world->set<CtxPipeline>(std::move(pipeline_builder),
                                std::move(pipeline));

        return empty_maybe{};
      },
      main_thread_task_list);
}

void EcsUtil::update_pipeline(indigo::igecs::WorldView* wv,
                              const wgpu::Device& device) {
  wgpu::TextureFormat hdr_format = wv->ctx<CtxHdrFramebufferParams>().format;
  auto& pipeline_ctx = wv->mut_ctx<CtxPipeline>();

  if (pipeline_ctx.currentPipeline.outputFormat != hdr_format) {
    pipeline_ctx.currentPipeline =
        pipeline_ctx.builder.create_pipeline(device, hdr_format);
  }
}

const Pipeline& EcsUtil::get_pipeline(indigo::igecs::WorldView* wv) {
  return wv->ctx<CtxPipeline>().currentPipeline;
}

ReadonlyResourceRegistry<Geo>::Key EcsUtil::register_geo(
    igecs::WorldView* wv, const wgpu::Device& device,
    const PodVector<asset::PositionNormalVertexData>& vertices,
    const PodVector<uint32_t>& indices) {
  return wv->mut_ctx<CtxResourceRegistries>().geoRegistry.add_resource(
      Geo(device, vertices, indices));
}

void EcsUtil::unregister_geo(igecs::WorldView* wv,
                             ReadonlyResourceRegistry<Geo>::Key key) {
  wv->mut_ctx<CtxResourceRegistries>().geoRegistry.remove_resource(key);
}

void EcsUtil::unregister_all_geo(igecs::WorldView* wv) {
  wv->mut_ctx<CtxResourceRegistries>().geoRegistry =
      ReadonlyResourceRegistry<Geo>();
}

// TODO (sessamekesh): Static geo is likely to stay the same on the GPU, instead
//  of storing all the data on every component, how about just storing what
//  changes have happened to it?
void EcsUtil::render_matching(igecs::WorldView* wv, const wgpu::Device& device,
                              RenderUtil* render_util,
                              std::function<bool(entt::entity)> predicate) {
  auto view = wv->view<const RenderableComponent, const MatWorldComponent>();
  auto& ctx_resource_registries = wv->mut_ctx<CtxResourceRegistries>();
  auto& instance_store = ctx_resource_registries.instanceStore;
  auto& geo_registry = ctx_resource_registries.geoRegistry;

  instance_store.begin_frame();

  for (auto [e, renderable, mat_world] : view.each()) {
    if (!predicate(e)) continue;

    InstanceData instance_data{};
    instance_data.matWorld = mat_world.matWorld;
    instance_data.albedo = renderable.albedo;
    instance_data.ambientOcclusion = renderable.ao;
    instance_data.metallic = renderable.metallic;
    instance_data.roughness = renderable.roughness;

    instance_store.add_instance(InstanceKey{renderable.geoKey}, instance_data);
  }

  instance_store.finalize(device, [render_util, &geo_registry](
                                      const auto& key,
                                      const wgpu::Buffer& buffer,
                                      uint32_t num_instances) {
    auto* geo = geo_registry.get(key.geo_key());
    if (!geo) {
      return;
    }

    render_util->set_geometry(*geo).set_instances(buffer, num_instances).draw();
  });
}
