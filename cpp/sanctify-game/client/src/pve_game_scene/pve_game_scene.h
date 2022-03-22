#ifndef SANCTIFY_GAME_CLIENT_SRC_PVE_GAME_SCENE_PVE_GAME_SCENE_H
#define SANCTIFY_GAME_CLIENT_SRC_PVE_GAME_SCENE_PVE_GAME_SCENE_H

#include <app_base.h>
#include <igasync/promise.h>
#include <igcore/maybe.h>
#include <iggpu/texture.h>
#include <ignav/detour_navmesh.h>
#include <net/reconcile_net_state_system.h>
#include <netclient/net_client.h>
#include <pve_game_scene/ecs/sim_time_sync_system.h>
#include <scene_base.h>
#include <util/registry_types.h>

#include <entt/entt.hpp>

namespace sanctify {

class PveGameScene : public ISceneBase,
                     public std::enable_shared_from_this<PveGameScene> {
 public:
  /** Determines how game ticks and messages are handled */
  enum class ClientStage {
    /** Client is not even ready to even show the loading state */
    Preload,

    /** Client is loading and/or server is in WaitingForPlayers state */
    Loading,

    /** Game simulation is running in earnest */
    Running,

    /** Server communication has been severed, but the scene persists */
    GameOver,
  };

 public:
  static std::shared_ptr<PveGameScene> Create(
      std::shared_ptr<AppBase> app_base,
      indigo::core::Maybe<std::shared_ptr<NetClient>> net_client,
      std::shared_ptr<indigo::core::TaskList> main_thread_task_list,
      std::shared_ptr<indigo::core::TaskList> async_task_list);

  // Return a promise that resolves as soon as the scene is prepared to
  //  be the active scene
  std::shared_ptr<indigo::core::Promise<indigo::core::EmptyPromiseRsl>>
  get_preload_finish_promise() const;

  // ISceneBase
  void update(float dt) override;
  void render() override;
  bool should_quit() override;
  void on_viewport_resize(uint32_t width, uint32_t height) override;

 private:
  PveGameScene(std::shared_ptr<AppBase> app_base,
               indigo::core::Maybe<std::shared_ptr<NetClient>> net_client,
               std::shared_ptr<indigo::core::TaskList> main_thread_task_list,
               std::shared_ptr<indigo::core::TaskList> async_task_list);

  void kick_off_preload();
  void kick_off_load();

  void loading_update(float dt);
  void loading_render();

  void running_update(float dt);
  void running_render();

  void game_over_update(float dt);
  void game_over_render();

  void handle_update_netsync(float dt);
  void update_server_ping(float dt);

  void setup_depth_texture(uint32_t width, uint32_t height);

  bool is_healthy() const;

  bool is_self_done_loading();

 private:
  ClientStage client_stage_;

  // Platform
  std::shared_ptr<AppBase> app_base_;
  std::shared_ptr<indigo::core::TaskList> main_thread_task_list_;
  std::shared_ptr<indigo::core::TaskList> async_task_list_;

  // Netstate - notice this is all empty/irrelevant for offline play
  indigo::core::Maybe<std::shared_ptr<NetClient>> net_client_;
  std::mutex m_net_message_queue_;
  indigo::core::Vector<pb::GameServerSingleMessage> net_message_queue_;
  SimTimeSyncSystem sim_time_sync_system_;

  // Bookkeeping
  std::shared_ptr<indigo::core::Promise<indigo::core::EmptyPromiseRsl>>
      on_preload_finish_;
  bool should_quit_;
  bool is_self_loaded_;
  float client_time_;
  float time_to_next_ping_;
  float time_since_last_server_message_;
  NetClient::ConnectionState connection_state_;

  // Simulation internals
  entt::registry world_;
  entt::entity client_state_entity_;

  // Non-ECS logical resources (navmeshes, spatial partitions, etc)
  indigo::core::Maybe<indigo::nav::DetourNavmesh> arena_navmesh_;

  // Targets
  indigo::iggpu::Texture depth_texture_;
  wgpu::TextureView depth_view_;

  // Render resources (loading screen)

  // Render resources (running / game over)
};

}  // namespace sanctify

#endif
