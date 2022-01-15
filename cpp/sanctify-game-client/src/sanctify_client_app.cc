#include <app_startup_scene/app_startup_scene.h>
#include <igcore/log.h>
#include <sanctify_client_app.h>

using namespace sanctify;

namespace {
const char* kLogLabel = "SanctifyClientApp";
}

std::shared_ptr<SanctifyClientApp> SanctifyClientApp::Create(uint32_t width,
                                                             uint32_t height) {
  auto app_base_rsl = AppBase::Create(width, height);
  if (app_base_rsl.is_right()) {
    Logger::err(kLogLabel) << "Failed to create AppBase - error "
                           << app_base_create_error_text(
                                  app_base_rsl.get_right());
    return nullptr;
  }
  auto app_base = app_base_rsl.left_move();

  auto main_thread_task_list = std::make_shared<TaskList>();

#ifdef IG_ENABLE_THREADS
  // Spin up async threads - no fancy pantsy scheduling schemes (yet), just
  //  have threads ready for async work. A more involved scheme may be necessary
  //  in the future.
  auto async_task_list = std::make_shared<TaskList>();

  core::Vector<std::shared_ptr<ExecutorThread>> executor_threads;
  for (int i = 1; i < std::thread::hardware_concurrency(); i++) {
    auto executor = std::make_shared<core::ExecutorThread>();
    executor->add_task_list(async_task_list);
    executor_threads.push_back(executor);
  }
#else
  // Threading is not supported - async task list is the main thread task list
  // in disguise!
  auto async_task_list = main_thread_task_list;
#endif

  auto main_thread_executor =
      std::make_shared<FrameTaskSchedulerExecutor>(main_thread_task_list);

#ifdef IG_ENABLE_THREADS
  auto app = std::shared_ptr<SanctifyClientApp>(new SanctifyClientApp(
      app_base, main_thread_task_list, main_thread_executor, executor_threads));
#else
  auto app = std::shared_ptr<SanctifyClientApp>(new SanctifyClientApp(
      app_base, main_thread_task_list, main_thread_executor));
#endif

  auto startup_scene = std::shared_ptr<AppStartupScene>(
      new AppStartupScene(app_base, app, main_thread_task_list));

  app->set_scene(startup_scene);
  app->swap_chain_format_ = app_base->preferred_swap_chain_texture_format();
  app->next_swap_chain_width_ = app->swap_chain_width_ = app_base->Width;
  app->next_swap_chain_height_ = app->swap_chain_height_ = app_base->Height;

  return app;
}

#ifdef IG_ENABLE_THREADS
SanctifyClientApp::SanctifyClientApp(
    std::shared_ptr<AppBase> app_base,
    std::shared_ptr<TaskList> main_thread_task_list,
    std::shared_ptr<FrameTaskSchedulerExecutor> main_thread_executor,
    core::Vector<std::shared_ptr<core::ExecutorThread>> executor_threads)
    : app_base_(app_base),
      active_scene_(nullptr),
      main_thread_task_list_(main_thread_task_list),
      executor_(main_thread_executor),
      swap_chain_width_(0u),
      swap_chain_height_(0u),
      next_swap_chain_width_(0u),
      next_swap_chain_height_(0u),
      swap_chain_format_(wgpu::TextureFormat::Undefined),
      time_to_next_swap_chain_update_(0.f),
      executor_threads_(std::move(executor_threads)) {}
#else
SanctifyClientApp::SanctifyClientApp(
    std::shared_ptr<AppBase> app_base,
    std::shared_ptr<TaskList> main_thread_task_list,
    std::shared_ptr<FrameTaskSchedulerExecutor> main_thread_executor)
    : app_base_(app_base),
      active_scene_(nullptr),
      main_thread_task_list_(main_thread_task_list),
      executor_(main_thread_executor),
      swap_chain_width_(0u),
      swap_chain_height_(0u),
      next_swap_chain_width_(0u),
      next_swap_chain_height_(0u),
      swap_chain_format_(wgpu::TextureFormat::Undefined),
      time_to_next_swap_chain_update_(0.f) {}
#endif

void SanctifyClientApp::set_scene(std::shared_ptr<ISceneBase> next_scene) {
  active_scene_ = next_scene;
}

void SanctifyClientApp::update(float dt) {
  time_to_next_swap_chain_update_ -= dt;
  active_scene_->update(dt);
}

void SanctifyClientApp::render() {
  int vp_width, vp_height;
  glfwGetFramebufferSize(app_base_->Window, &vp_width, &vp_height);

  if (vp_width != next_swap_chain_width_ ||
      vp_height != next_swap_chain_height_) {
    next_swap_chain_width_ = vp_width;
    next_swap_chain_height_ = vp_height;
    time_to_next_swap_chain_update_ = 0.6f;
  }
  if ((next_swap_chain_width_ != swap_chain_width_ ||
       next_swap_chain_height_ != swap_chain_height_) &&
      time_to_next_swap_chain_update_ < 0.f) {
    Logger::log(kLogLabel) << "Resizing swap chain from " << swap_chain_width_
                           << "x" << swap_chain_height_ << " to "
                           << next_swap_chain_width_ << "x"
                           << next_swap_chain_height_;

    swap_chain_width_ = next_swap_chain_width_;
    swap_chain_height_ = next_swap_chain_height_;
    // TODO (sessamekesh): Re-enable this when it becomes clear why this is
    // freaking out on your old shitty laptop (which needs to be supported)
    // demo_base_->resize_swap_chain(next_swap_chain_width_,
    //                              next_swap_chain_height_);
    active_scene_->on_viewport_resize(swap_chain_width_, swap_chain_height_);
  }

  active_scene_->render();
}

void SanctifyClientApp::run_tasks_for(float dt_s) {
  {
    auto start = std::chrono::high_resolution_clock::now();
    auto end = start + std::chrono::microseconds((uint32_t)(dt_s * 1000000.f));
    while (main_thread_task_list_->execute_next()) {
      if (std::chrono::high_resolution_clock::now() > end) {
        break;
      }
    }
  }

  executor_->tick();
}

bool SanctifyClientApp::should_quit() {
  return active_scene_->should_quit() ||
         glfwWindowShouldClose(app_base_->Window);
}