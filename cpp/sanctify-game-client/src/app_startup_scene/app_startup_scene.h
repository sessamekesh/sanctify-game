#ifndef SANCTIFY_GAME_CLIENT_SRC_APP_STARTUP_SCENE_APP_STARTUP_SCENE_H
#define SANCTIFY_GAME_CLIENT_SRC_APP_STARTUP_SCENE_APP_STARTUP_SCENE_H

#include <app_startup_scene/startup_shader_src.h>
#include <igasync/promise.h>
#include <iggpu/ubo_base.h>
#include <scene_base.h>

using namespace indigo;
using namespace core;

namespace sanctify {

class AppStartupScene : public ISceneBase {
 public:
  AppStartupScene(std::shared_ptr<AppBase> base,
                  std::shared_ptr<ISceneConsumer> scene_consumer,
                  std::shared_ptr<TaskList> main_thread_task_list);

  void update(float dt) override;
  void render() override;
  bool should_quit() override;

  void setup_render_state(const AppBase& base);

 private:
  std::shared_ptr<AppBase> base_;
  std::shared_ptr<ISceneConsumer> scene_consumer_;
  std::shared_ptr<TaskList> main_thread_task_list_;

  wgpu::RenderPipeline render_pipeline_;
  iggpu::UboBase<FrameParams> ubo_;
  wgpu::BindGroup bind_group_;
};

}  // namespace sanctify

#endif
