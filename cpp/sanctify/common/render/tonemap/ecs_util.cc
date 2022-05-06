#include "ecs_util.h"

#include <common/render/common/render_components.h>
#include <igasync/promise_combiner.h>

using namespace sanctify;
using namespace render;
using namespace tonemap;
using namespace indigo;
using namespace core;

namespace {
const char* kLogLabel = "s::r::tonemap::EcsUtil";
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

        auto pipeline_builder = iggpu::PipelineBuilder::Create(
            device, vs_rsl.get_left(), fs_rsl.get_left());

        // TODO (sessamekesh): Everything above here can be made a general
        //  promise for PipelineBuilder construction, nothing unique so far.

        wgpu::TextureFormat swap_chain_format =
            world->ctx<CtxPlatformObjects>().swapChainFormat;

        auto pipeline =
            Pipeline::FromBuilder(device, pipeline_builder, swap_chain_format);

        world->set<CtxPipeline>(std::move(pipeline_builder),
                                std::move(pipeline));

        return empty_maybe{};
      },
      main_thread_task_list);
}

void EcsUtil::update_pipeline(indigo::igecs::WorldView* wv) {
  const auto& platform_objects = wv->ctx<CtxPlatformObjects>();
  auto& pipeline_ctx = wv->mut_ctx<CtxPipeline>();

  if (pipeline_ctx.currentPipeline.outputFormat !=
      platform_objects.swapChainFormat) {
    pipeline_ctx.currentPipeline =
        Pipeline::FromBuilder(platform_objects.device, pipeline_ctx.builder,
                              platform_objects.swapChainFormat);
  }
}
