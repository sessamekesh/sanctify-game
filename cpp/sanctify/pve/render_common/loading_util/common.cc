#include "common.h"

#include <common/render/common/render_components.h>
#include <common/render/solid_static/ecs_util.h>
#include <common/render/viewport/update_arena_camera_system.h>
#include <igecs/world_view.h>

using namespace sanctify;
using namespace pve;
using namespace indigo;
using namespace core;

std::shared_ptr<Promise<Maybe<std::string>>>
CommonRenderLoadingUtil::setup_ubos_and_hdr_state(
    const wgpu::Device& device, entt::registry* world,
    std::shared_ptr<TaskList> main_thread_task_list, uint32_t client_width,
    uint32_t client_height, wgpu::TextureFormat hdr_format) {
  return Promise<Maybe<std::string>>::schedule(
      main_thread_task_list,
      [device, world, hdr_format, client_width,
       client_height]() -> Maybe<std::string> {
        world->set<render::CtxHdrFramebufferParams>(hdr_format, client_width,
                                                    client_height);

        // Thin world view allowed for main thread task list
        auto wv = igecs::WorldView::Thin(world);
        if (!render::solid_static::EcsUtil::prepare_world(&wv)) {
          return {"Failed to prepare solid_static world state"};
        }

        world->set<render::CtxMainCameraCommonUbos>(
            render::CameraCommonVsUbo(device),
            render::CameraCommonFsUbo(device),
            render::CommonLightingUbo(device));

        return empty_maybe{};
      });
}

std::shared_ptr<Promise<Maybe<render::PipelineBuildError>>>
CommonRenderLoadingUtil::build_solid_shader_pipeline(
    const wgpu::Device& device, entt::registry* world,
    std::shared_ptr<Promise<Maybe<std::string>>> hdr_promise,
    std::shared_ptr<TaskList> main_thread_task_list,
    std::shared_ptr<TaskList> async_task_list,
    asset::IgpackLoader* shader_resource_file, std::string vs_key,
    std::string fs_key) {
  auto vs_promise =
      shader_resource_file->extract_wgsl_shader(vs_key, async_task_list);
  auto fs_promise =
      shader_resource_file->extract_wgsl_shader(fs_key, async_task_list);

  return hdr_promise->then_chain<Maybe<render::PipelineBuildError>>(
      [world, device, main_thread_task_list, vs_promise,
       fs_promise](const auto& hdr_rsl)
          -> std::shared_ptr<Promise<Maybe<render::PipelineBuildError>>> {
        if (hdr_rsl.has_value()) {
          return Promise<Maybe<render::PipelineBuildError>>::immediate(
              render::PipelineBuildError::PipelineBuildError);
        }

        return render::solid_static::EcsUtil::create_ctx_pipeline(
            world, device, vs_promise, fs_promise, main_thread_task_list);
      },
      main_thread_task_list);
}
