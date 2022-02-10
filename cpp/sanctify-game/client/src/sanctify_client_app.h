#ifndef SANCTIFY_GAME_CLIENT_SRC_SANCTIFY_CLIENT_APP_H
#define SANCTIFY_GAME_CLIENT_SRC_SANCTIFY_CLIENT_APP_H

#include <app_base.h>
#include <igasync/frame_task_scheduler.h>
#include <igasync/task_list.h>
#include <igcore/either.h>
#include <scene_base.h>

#ifdef IG_ENABLE_THREADS
#include <igasync/executor_thread.h>
#endif

using namespace indigo;
using namespace core;

namespace sanctify {

class SanctifyClientApp
    : public ISceneConsumer,
      public std::enable_shared_from_this<SanctifyClientApp> {
 public:
  // TODO (sessamekesh): Instead of injecting int params, inject in a
  // configuration object (JSON or protobuf - probably protobuf)
  static std::shared_ptr<SanctifyClientApp> Create(uint32_t width = 0,
                                                   uint32_t height = 0);

  void set_scene(std::shared_ptr<ISceneBase> next_scene) override;

  // Expose to app entry point
  void update(float dt);
  void render();
  void run_tasks_for(float dt_s);
  bool should_quit();

 private:
#ifdef IG_ENABLE_THREADS
  SanctifyClientApp(
      std::shared_ptr<AppBase> app_base,
      std::shared_ptr<TaskList> main_thread_task_list,
      std::shared_ptr<core::FrameTaskSchedulerExecutor> executor,
      core::Vector<std::shared_ptr<core::ExecutorThread>> executor_threads);
#else
  SanctifyClientApp(std::shared_ptr<AppBase> app_base,
                    std::shared_ptr<TaskList> main_thread_task_list,
                    std::shared_ptr<core::FrameTaskSchedulerExecutor> executor);
#endif

 private:
  std::shared_ptr<AppBase> app_base_;
  std::shared_ptr<ISceneBase> active_scene_;

  std::shared_ptr<TaskList> main_thread_task_list_;
  std::shared_ptr<core::FrameTaskSchedulerExecutor> executor_;

  // Swap chain information
  uint32_t swap_chain_width_, swap_chain_height_;
  uint32_t next_swap_chain_width_, next_swap_chain_height_;
  wgpu::TextureFormat swap_chain_format_;
  float time_to_next_swap_chain_update_;

#ifdef IG_ENABLE_THREADS
  core::Vector<std::shared_ptr<core::ExecutorThread>> executor_threads_;
#endif
};

}  // namespace sanctify

#endif
