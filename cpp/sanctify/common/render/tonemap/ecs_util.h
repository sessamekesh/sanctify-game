#ifndef SANCTIFY_COMMON_RENDER_TONEMAP_ECS_UTIL_H
#define SANCTIFY_COMMON_RENDER_TONEMAP_ECS_UTIL_H

#include <common/render/common/pipeline_build_error.h>
#include <igasset/igpack_loader.h>
#include <igecs/world_view.h>

#include <entt/entt.hpp>

#include "pipeline.h"

namespace sanctify::render::tonemap {

struct CtxPipeline {
  indigo::iggpu::PipelineBuilder builder;
  Pipeline currentPipeline;
};

/** Utility methods for dealing with Tonemap rendering */
struct EcsUtil {
  // Pipeline creation and fetching
  static std::shared_ptr<
      indigo::core::Promise<indigo::core::Maybe<PipelineBuildError>>>
  create_ctx_pipeline(
      entt::registry* world, const wgpu::Device& device,
      indigo::asset::IgpackLoader::ExtractWgslShaderPromiseT vs_src_promise,
      indigo::asset::IgpackLoader::ExtractWgslShaderPromiseT fs_src_promise,
      std::shared_ptr<indigo::core::TaskList> main_thread_task_list);

  static void update_pipeline(indigo::igecs::WorldView* wv);
};

}  // namespace sanctify::render::tonemap

#endif
