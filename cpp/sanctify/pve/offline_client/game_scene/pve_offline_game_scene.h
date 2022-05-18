#ifndef SANCTIFY_PVE_OFFLINE_CLIENT_GAME_SCENE_PVE_OFFLINE_GAME_SCENE_H
#define SANCTIFY_PVE_OFFLINE_CLIENT_GAME_SCENE_PVE_OFFLINE_GAME_SCENE_H

#include <common/scene/scene_base.h>
#include <common/simple_client_app/simple_client_app_base.h>
#include <igasync/promise.h>
#include <igcore/either.h>
#include <igecs/scheduler.h>
#include <pve/offline_client/pb/pve_offline_client_config.pb.h>

#include <entt/entt.hpp>
#include <memory>

namespace sanctify::pve {

class PveOfflineGameScene : public ISceneBase,
                            std::enable_shared_from_this<PveOfflineGameScene> {
 public:
  static std::shared_ptr<indigo::core::Promise<
      indigo::core::Either<std::shared_ptr<ISceneBase>, std::string>>>
  Create(std::shared_ptr<SimpleClientAppBase> app_base,
         std::shared_ptr<indigo::core::TaskList> main_thread_task_list,
         pb::PveOfflineClientConfig client_config);

  // ISceneBase
  void update(float dt) override;
  void render() override;
  bool should_quit() override;
  void on_viewport_resize(uint32_t width, uint32_t height) override;
  void on_swap_chain_format_change(wgpu::TextureFormat format) override;
  void consume_event(io::Event evt) override;

 private:
  PveOfflineGameScene(
      std::shared_ptr<SimpleClientAppBase> app_base,
      std::shared_ptr<indigo::core::TaskList> main_thread_task_list,
      std::shared_ptr<indigo::core::TaskList> async_task_list,
      pb::PveOfflineClientConfig config);

  // System essentials
  std::shared_ptr<SimpleClientAppBase> base_;
  std::shared_ptr<indigo::core::TaskList> main_thread_task_list_;
  std::shared_ptr<indigo::core::TaskList> async_task_list_;

  // Simulation essentials
  entt::registry client_world_;
  entt::registry server_world_;

  std::shared_ptr<indigo::core::TaskList> any_thread_task_list_;
  indigo::igecs::Scheduler update_client_scheduler_;
  indigo::igecs::Scheduler render_client_scheduler_;

  // Miscellaneous
  pb::PveOfflineClientConfig config_;
  bool should_quit_;
};

}  // namespace sanctify::pve

#endif
