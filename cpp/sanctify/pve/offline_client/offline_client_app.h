#ifndef SANCTIFY_PVE_OFFLINE_CLIENT_OFFLINE_CLIENT_APP_H
#define SANCTIFY_PVE_OFFLINE_CLIENT_OFFLINE_CLIENT_APP_H

#include <common/scene/preload/preload_scene.h>
#include <common/scene/scene_base.h>
#include <common/simple_client_app/simple_client_app_base.h>
#include <common/user_input/events.h>
#include <igasync/task_list.h>
#include <pve/offline_client/pb/pve_offline_client_config.pb.h>

#include <memory>

namespace sanctify::pve {

class OfflineClientApp : public std::enable_shared_from_this<OfflineClientApp>,
                         public sanctify::ISceneConsumer {
 public:
  static std::shared_ptr<OfflineClientApp> Create(std::string config_string);

  void set_scene(std::shared_ptr<ISceneBase> next_scene) override;

  // Expose to app entry point
  void update(float dt);
  void render();
  void run_tasks_for(float dt);
  bool should_quit();

  // Standard inputs
  void mouse_move(io::MouseMoveEvent evt);
  void focus_change(io::FocusChangeEvent evt);

  // Expose GLFW window for native builds
  GLFWwindow* get_window() const;

 private:
  OfflineClientApp(
      std::shared_ptr<SimpleClientAppBase> base,
      std::shared_ptr<indigo::core::TaskList> main_thread_task_list,
      std::shared_ptr<indigo::core::TaskList> async_task_list,
      pb::PveOfflineClientConfig client_config);

 private:
  std::shared_ptr<SimpleClientAppBase> base_;
  std::shared_ptr<ISceneBase> active_scene_;

  std::shared_ptr<indigo::core::TaskList> main_thread_task_list_;
  std::shared_ptr<indigo::core::TaskList> async_task_list_;

  // Swap chain information
  uint32_t vp_width_, vp_height_;
  uint32_t next_vp_width_, next_vp_height_;
  wgpu::TextureFormat vp_format_;
  float time_to_swap_chain_update_;

  // Config
  pb::PveOfflineClientConfig client_config_;
};

}  // namespace sanctify::pve

#endif
