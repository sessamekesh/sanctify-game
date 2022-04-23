#include "offline_client_app.h"

#include <igcore/either.h>
#include <igcore/log.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/html5.h>
#endif

using namespace sanctify;
using namespace pve;
using namespace indigo;
using namespace core;

namespace {
const char* kLogLabel = "SanctifyOfflineClientApp";
}

std::shared_ptr<OfflineClientApp> OfflineClientApp::Create(
    std::string config_string) {
  pb::PveOfflineClientConfig config{};
  if (!config.ParseFromString(config_string)) {
    Logger::err(kLogLabel) << "Failed to parse config string";
    return nullptr;
  }

  Logger::log(kLogLabel) << "Creating a new OfflineClientApp! This should only "
                            "happen once per application lifetime";

  auto app_base_rsl =
      SimpleClientAppBase::Create("Sanctify PvE Offline Client");

  if (app_base_rsl.is_right()) {
    Logger::err(kLogLabel) << "Failed to create AppBase - error "
                           << to_string(app_base_rsl.get_right());
    return nullptr;
  }

  auto app_base = app_base_rsl.left_move();

  auto main_thread_task_list = std::make_shared<TaskList>();
  std::shared_ptr<TaskList> async_task_list = main_thread_task_list;

  if (app_base->supports_threads()) {
    async_task_list = std::make_shared<TaskList>();
    app_base->attach_async_task_list(async_task_list);
  }

  auto app = std::shared_ptr<OfflineClientApp>(new OfflineClientApp(
      app_base, main_thread_task_list, async_task_list, config));

  // TODO (sessamekesh): Get loading scene here instead of this one, which will
  //  never be resolved and allows the preload scene to just... spin
  auto next_scene_promise =
      Promise<Either<std::shared_ptr<ISceneBase>, std::string>>::create();

  auto startup_scene = std::shared_ptr<PreloadScene>(
      new PreloadScene(app_base->device, app_base->swapChain, app_base->width,
                       app_base->height, app_base->swapChainFormat, app,
                       main_thread_task_list, next_scene_promise));

  app->set_scene(startup_scene);

  return app;
}

OfflineClientApp::OfflineClientApp(
    std::shared_ptr<SimpleClientAppBase> base,
    std::shared_ptr<TaskList> main_thread_task_list,
    std::shared_ptr<TaskList> async_task_list,
    pb::PveOfflineClientConfig client_config)
    : base_(base),
      active_scene_(nullptr),
      main_thread_task_list_(main_thread_task_list),
      async_task_list_(async_task_list),
      vp_width_(base->width),
      vp_height_(base->height),
      next_vp_width_(base->width),
      next_vp_height_(base->height),
      vp_format_(base->swap_chain_format()),
      client_config_(client_config),
      time_to_swap_chain_update_(0.f) {}

void OfflineClientApp::set_scene(std::shared_ptr<ISceneBase> next_scene) {
  active_scene_ = next_scene;
}

void OfflineClientApp::update(float dt) {
  time_to_swap_chain_update_ -= dt;
  active_scene_->update(dt);
}

void OfflineClientApp::render() {
  int vp_w = 0, vp_h = 0;
#ifdef EMSCRIPTEN
  emscripten_get_canvas_element_size("#app_canvas", &vp_w, &vp_h);
#else
  glfwGetFramebufferSize(base_->window, &vp_w, &vp_h);
#endif

  if (vp_w != next_vp_width_ || vp_h != next_vp_height_) {
    next_vp_width_ = vp_w;
    next_vp_height_ = vp_h;
    time_to_swap_chain_update_ =
        client_config_.render_settings().swap_chain_resize_latency();
  }
  if ((next_vp_width_ != vp_width_ || next_vp_height_ != vp_height_) &&
      time_to_swap_chain_update_ < 0.f) {
    Logger::log(kLogLabel) << "Resizing swap chain from " << vp_width_ << "x"
                           << vp_height_ << " to " << next_vp_width_ << "x"
                           << next_vp_height_;

    vp_width_ = next_vp_width_;
    vp_height_ = next_vp_height_;

    base_->resize_swap_chain(vp_width_, vp_height_);
    active_scene_->on_viewport_resize(vp_width_, vp_height_);
  }

  active_scene_->render();
}

void OfflineClientApp::run_tasks_for(float dt) {
  using FpSeconds = std::chrono::duration<float, std::chrono::seconds::period>;
  auto start = std::chrono::high_resolution_clock::now();
  auto end = start + FpSeconds(dt);

  while (main_thread_task_list_->execute_next()) {
    if (std::chrono::high_resolution_clock::now() > end) {
      break;
    }
  }
}

bool OfflineClientApp::should_quit() {
  return active_scene_->should_quit() || glfwWindowShouldClose(base_->window);
}
