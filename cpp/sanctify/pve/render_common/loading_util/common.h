#ifndef SANCTIFY_PVE_RENDER_COMMON_LOADING_UTIL_COMMON_H
#define SANCTIFY_PVE_RENDER_COMMON_LOADING_UTIL_COMMON_H

#include <common/render/common/pipeline_build_error.h>
#include <igasset/igpack_loader.h>
#include <igasync/promise.h>
#include <igcore/maybe.h>
#include <webgpu/webgpu_cpp.h>

#include <entt/entt.hpp>

namespace sanctify::pve {

class CommonRenderLoadingUtil {
 public:
  // Setup frame common resources in a main thread task:
  // - HDR framebuffer global state
  // - Shared UBOs for 3D camera / lighting params
  static std::shared_ptr<
      indigo::core::Promise<indigo::core::Maybe<std::string>>>
  setup_ubos_and_hdr_state(
      const wgpu::Device& device, entt::registry* world,
      std::shared_ptr<indigo::core::TaskList> main_thread_task_list,
      uint32_t client_width, uint32_t client_height,
      wgpu::TextureFormat hdr_format);

  // Load solid shader (depends on HDR state)
  static std::shared_ptr<
      indigo::core::Promise<indigo::core::Maybe<render::PipelineBuildError>>>
  build_solid_shader_pipeline(
      const wgpu::Device& device, entt::registry* world,
      std::shared_ptr<indigo::core::Promise<indigo::core::Maybe<std::string>>>
          hdr_promise,
      std::shared_ptr<indigo::core::TaskList> main_thread_task_list,
      std::shared_ptr<indigo::core::TaskList> async_task_list,
      indigo::asset::IgpackLoader* shader_resource_file, std::string vs_key,
      std::string fs_key);
};

}  // namespace sanctify::pve

#endif
