#include "pve_offline_game_scene.h"

#include <common/logic/update_common/tick_time_elapsed.h>
#include <common/render/common/camera_ubos.h>
#include <common/render/common/render_components.h>
#include <common/render/solid_static/ecs_util.h>
#include <common/render/viewport/update_arena_camera_system.h>
#include <igasset/igpack_loader.h>
#include <igasync/promise_combiner.h>
#include <igecs/scheduler.h>
#include <pve/render_common/loading_util/common.h>

#include "render_client_scheduler.h"
#include "update_client_scheduler.h"

using namespace sanctify;
using namespace pve;
using namespace indigo;
using namespace core;

////////////////////////////////////////////////////////////////////////////////////
//                               SCENE LOADING
// TODO (sessamekesh): Move this out into a separate file! It's cluttered here!!
////////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<Promise<Either<std::shared_ptr<ISceneBase>, std::string>>>
PveOfflineGameScene::Create(std::shared_ptr<SimpleClientAppBase> app_base,
                            std::shared_ptr<TaskList> main_thread_task_list,
                            pb::PveOfflineClientConfig client_config) {
  auto async_task_list = std::make_shared<TaskList>();
  if (app_base->supports_threads()) {
    app_base->attach_async_task_list(async_task_list);
  } else {
    async_task_list = main_thread_task_list;
  }

  auto game_scene =
      std::shared_ptr<PveOfflineGameScene>(new PveOfflineGameScene(
          app_base, main_thread_task_list, async_task_list, client_config));

  asset::IgpackLoader shader_loader("resources/common-shaders.igpack",
                                    async_task_list);
  asset::IgpackLoader arena_base_loader("resources/arena-base.igpack",
                                        async_task_list);

  const wgpu::Device& device = game_scene->base_->device;
  entt::registry& client_world = game_scene->client_world_;
  auto width = game_scene->base_->width;
  auto height = game_scene->base_->height;

  auto* world = &game_scene->client_world_;
  auto wv = igecs::WorldView::Thin(world);

  world->set<render::CtxPlatformObjects>(
      device, app_base->swapChainFormat,
      app_base->swapChain.GetCurrentTextureView(), app_base->width,
      app_base->height);

  auto common_ubo_hdr_promise =
      CommonRenderLoadingUtil::setup_ubos_and_hdr_state(
          device, &client_world, main_thread_task_list, width, height,
          wgpu::TextureFormat::RGBA16Float);
  auto solid_static_pipeline_promise =
      CommonRenderLoadingUtil::build_solid_shader_pipeline(
          device, &client_world, common_ubo_hdr_promise, main_thread_task_list,
          async_task_list, &shader_loader, "solidStaticVs", "solidStaticFs");
  auto tonemap_pipeline_promise =
      CommonRenderLoadingUtil ::build_tonemapping_shader_pipeline(
          device, &client_world, main_thread_task_list, async_task_list,
          &shader_loader, "tonemapVs", "tonemapFs");

  auto load_terrain_base_promise =
      arena_base_loader.extract_draco_geo("terrainBaseGeo", async_task_list)
          ->then_chain<Maybe<std::string>>(
              [world, device, main_thread_task_list](
                  const asset::IgpackLoader::ExtractDracoBufferT& rsl) {
                if (rsl.is_right()) {
                  return Promise<Maybe<std::string>>::immediate(
                      {"Failed to load Draco result"});
                }

                const auto& decoder = rsl.get_left();

                auto pos_norm_rsl = decoder->get_pos_norm_data();
                auto indices_rsl = decoder->get_index_data();

                if (pos_norm_rsl.is_right() || indices_rsl.is_right()) {
                  return Promise<Maybe<std::string>>::immediate(
                      {"Failed to extract pos/norm/indices from Draco"});
                }

                return Promise<Maybe<std::string>>::schedule(
                    main_thread_task_list,
                    [world, pn = pos_norm_rsl.left_move(),
                     idc = indices_rsl.left_move(), device]() {
                      auto wv = indigo::igecs::WorldView::Thin(world);

                      auto geo_key =
                          render::solid_static::EcsUtil::register_geo(
                              &wv, device, pn, idc);

                      render::solid_static::EcsUtil::attach_renderable(
                          &wv, wv.create(), geo_key,
                          render::solid_static::InstanceData{
                              glm::mat4(1.f), glm::vec3(1.f, 1.f, 1.f), 0.01f,
                              0.3f, 0.f});

                      return empty_maybe{};
                    });
              },
              async_task_list);

  auto combiner = PromiseCombiner::Create();

  auto solid_static_shader_rsl_key =
      combiner->add(solid_static_pipeline_promise, async_task_list);
  auto tonemap_shader_rsl_key =
      combiner->add(tonemap_pipeline_promise, async_task_list);
  auto load_terrain_base_key =
      combiner->add(load_terrain_base_promise, async_task_list);

  return combiner->combine<Either<std::shared_ptr<ISceneBase>, std::string>>(
      [game_scene, solid_static_shader_rsl_key, load_terrain_base_key,
       tonemap_shader_rsl_key](PromiseCombiner::PromiseCombinerResult rsl)
          -> Either<std::shared_ptr<ISceneBase>, std::string> {
        if (rsl.get(solid_static_shader_rsl_key).has_value()) {
          return right<std::string>("Failed to load the SolidStaticShader");
        }

        if (rsl.get(load_terrain_base_key).has_value()) {
          return right<std::string>("Failed to load terrain geo");
        }

        if (rsl.get(tonemap_shader_rsl_key).has_value()) {
          return right<std::string>("Failed to load tonemapping shader");
        }

        // TODO (sessamekesh): Move this to a different load step
        auto wv = indigo::igecs::WorldView::Thin(&game_scene->client_world_);
        render::UpdateArenaCameraSystem::set_camera(
            &wv,
            logic::ArenaCamera(glm::vec3(0.f, 0.f, 0.f), glm::radians(35.f),
                               glm::radians(45.f), 45.f),
            0.1f, 1000.f, glm::radians(40.f));
        render::UpdateArenaCameraSystem::set_lighting(
            &wv, render::CommonLightingParamsData{
                     glm::normalize(glm::vec3(1.f, -4.f, 1.2f)), 0.45f,
                     glm::vec3(1.f, 1.f, 1.f), 20.f});

        Logger::log("PveOfflineClient") << "Loading finished!";
        return left<std::shared_ptr<ISceneBase>>(game_scene);
      },
      main_thread_task_list);
}

PveOfflineGameScene::PveOfflineGameScene(
    std::shared_ptr<SimpleClientAppBase> app_base,
    std::shared_ptr<TaskList> main_thread_task_list,
    std::shared_ptr<TaskList> async_task_list,
    pb::PveOfflineClientConfig config)
    : base_(app_base),
      main_thread_task_list_(main_thread_task_list),
      any_thread_task_list_(std::make_shared<TaskList>()),
      async_task_list_(async_task_list),
      config_(std::move(config)),
      should_quit_(false),
      update_client_scheduler_(pve::UpdateClientScheduler::build()),
      render_client_scheduler_(pve::build_render_client_scheduler()) {}

///////////////////////////////////////////////////////////////////////////////////////
//                                  SCENE UPDATING
///////////////////////////////////////////////////////////////////////////////////////

void PveOfflineGameScene::update(float dt) {
  // TODO (sessamekesh): Get rid of this dependency - or, better yet, move
  //  camera stuff over to render.
  const auto& device = base_->device;
  client_world_.set<render::CtxPlatformObjects>(
      device, base_->swapChainFormat, base_->swapChain.GetCurrentTextureView(),
      base_->width, base_->height);
  indigo::igecs::WorldView thinview =
      indigo::igecs::WorldView::Thin(&client_world_);
  logic::FrameTimeElapsedUtil::mark_time_elapsed(&thinview, dt);

  base_->attach_async_task_list(any_thread_task_list_);
  update_client_scheduler_.execute(any_thread_task_list_, &client_world_);
  base_->detach_async_task_list(any_thread_task_list_);
}

void PveOfflineGameScene::render() {
  const auto& device = base_->device;

  client_world_.set<render::CtxPlatformObjects>(
      device, base_->swapChainFormat, base_->swapChain.GetCurrentTextureView(),
      base_->width, base_->height);

  base_->attach_async_task_list(any_thread_task_list_);
  render_client_scheduler_.execute(any_thread_task_list_, &client_world_);
  base_->detach_async_task_list(any_thread_task_list_);

  base_->swapChain.Present();
}

bool PveOfflineGameScene::should_quit() { return should_quit_; }

void PveOfflineGameScene::on_viewport_resize(uint32_t width, uint32_t height) {
  auto& hdr_framebuffer_params =
      client_world_.ctx<render::CtxHdrFramebufferParams>();
  hdr_framebuffer_params.width = width;
  hdr_framebuffer_params.height = height;
}

void PveOfflineGameScene::on_swap_chain_format_change(
    wgpu::TextureFormat format) {}

void PveOfflineGameScene::consume_event(io::Event evt) {
  auto wv = igecs::WorldView::Thin(&client_world_);
  io::EcsUtil::add_event(&wv, evt);
}
