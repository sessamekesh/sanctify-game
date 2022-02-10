#ifndef SANCTIFY_GAME_CLIENT_SRC_GAME_SCENE_GAME_SCENE_H
#define SANCTIFY_GAME_CLIENT_SRC_GAME_SCENE_GAME_SCENE_H

#include <game_scene/net/reconcile_net_state_system.h>
#include <game_scene/net/snapshot_cache.h>
#include <game_scene/systems/player_move_indicator_render_system.h>
#include <game_scene/systems/player_render_system.h>
#include <igcore/vector.h>
#include <iggpu/texture.h>
#include <io/arena_camera_controller/arena_camera_input.h>
#include <io/viewport_click/viewport_click_controller_input.h>
#include <netclient/net_client.h>
#include <render/camera/arena_camera.h>
#include <render/debug_geo/debug_geo.h>
#include <render/debug_geo/debug_geo_pipeline.h>
#include <render/solid_animated/solid_animated_pipeline.h>
#include <render/terrain/terrain_geo.h>
#include <render/terrain/terrain_pipeline.h>
#include <sanctify-game-common/gameplay/locomotion.h>
#include <scene_base.h>
#include <webgpu/webgpu_cpp.h>

#include <entt/entt.hpp>

namespace sanctify {

class GameScene : public ISceneBase,
                  public std::enable_shared_from_this<GameScene> {
 public:
  struct TerrainShit {
    terrain_pipeline::TerrainPipelineBuilder PipelineBuilder;
    terrain_pipeline::TerrainPipeline Pipeline;
    terrain_pipeline::FramePipelineInputs FrameInputs;
    terrain_pipeline::ScenePipelineInputs SceneInputs;
    terrain_pipeline::MaterialPipelineInputs MaterialInputs;
    terrain_pipeline::TerrainGeo BaseGeo;
    terrain_pipeline::TerrainGeo DecorationGeo;
    terrain_pipeline::TerrainMatWorldInstanceBuffer IdentityBuffer;
  };

  // TODO (sessamekesh): Consolidate identical bind groups (frame/scene
  //  inputs can certainly be shared - allow constructors of these UBOs
  //  to take in existing buffers)
  struct PlayerShit {
    solid_animated::SolidAnimatedPipelineBuilder PipelineBuilder;
    solid_animated::SolidAnimatedPipeline Pipeline;
    solid_animated::FramePipelineInputs FrameInputs;
    solid_animated::ScenePipelineInputs SceneInputs;

    solid_animated::MaterialPipelineInputs BaseMaterial;
    solid_animated::MaterialPipelineInputs JointsMaterial;

    solid_animated::SolidAnimatedGeo BaseGeo;
    solid_animated::SolidAnimatedGeo JointsGeo;

    solid_animated::MatWorldInstanceBuffer InstanceBuffers;
  };

  // TODO (sessamekesh): Consolidate identical bind groups even more!!!
  //  You'll probably need to stop using the UboBase concept for these
  //  shared buffers like camera common and lighting common.
  struct DebugGeoShit {
    debug_geo::DebugGeoPipelineBuilder pipelineBuilder;
    debug_geo::DebugGeoPipeline pipeline;
    debug_geo::FramePipelineInputs frameInputs;
    debug_geo::ScenePipelineInputs sceneInputs;

    debug_geo::DebugGeo cubeGeo;

    debug_geo::InstanceBuffer cubeInstanceBuffer;
  };

 public:
  static std::shared_ptr<GameScene> Create(
      std::shared_ptr<AppBase> base, ArenaCamera arena_camera,
      std::shared_ptr<IArenaCameraInput> camera_input_system,
      std::shared_ptr<ViewportClickInput> viewport_click_info,
      TerrainShit terrain_shit, PlayerShit player_shit,
      DebugGeoShit debug_geo_shit, std::shared_ptr<NetClient> net_client,
      float camera_movement_speed, float fovy);
  ~GameScene();

  // ISceneBase
  void update(float dt) override;
  void render() override;
  bool should_quit() override;
  void on_viewport_resize(uint32_t width, uint32_t height) override;

 private:
  GameScene(std::shared_ptr<AppBase> base, ArenaCamera arena_camera,
            std::shared_ptr<IArenaCameraInput> camera_input_system,
            std::shared_ptr<ViewportClickInput> viewport_click_info,
            TerrainShit terrain_shit, PlayerShit player_shit,
            DebugGeoShit debug_geo_shit, std::shared_ptr<NetClient> net_client,
            float camera_movement_speed, float fovy);
  void post_ctor_setup();

  void setup_depth_texture(uint32_t width, uint32_t height);

  void handle_server_events();
  void handle_single_message(const pb::GameServerSingleMessage& msg);

  void dispatch_client_messages();
  void reconcile_net_state();

  void advance_simulation(entt::registry& world, float dt);

 private:
  std::shared_ptr<AppBase> base_;

  indigo::iggpu::Texture depth_texture_;
  wgpu::TextureView depth_view_;

  std::shared_ptr<IArenaCameraInput> arena_camera_input_;
  std::shared_ptr<ViewportClickInput> viewport_click_info_;
  float camera_movement_speed_;
  float fovy_;
  ArenaCamera arena_camera_;

  // Random shit that should be replaced with better, more fleshed out systems
  // You'll need a scene graph and renderable handle system, especially when
  // dealing with Lua, and might as well put in some simple AABB/frustum culling
  TerrainShit terrain_shit_;
  PlayerShit player_shit_;
  DebugGeoShit debug_geo_shit_;

  std::shared_ptr<NetClient> net_client_;

  // Game state...
  entt::registry world_;
  float client_clock_;

  // Net state...
  float server_clock_;
  system::PlayerRenderSystem player_render_system_;
  PlayerMoveIndicatorRenderSystem player_move_indicator_render_system_;
  SnapshotCache snapshot_cache_;
  ReconcileNetStateSystem reconcile_net_state_system_;

  std::mutex mut_connection_state_;
  NetClient::ConnectionState connection_state_;
  std::mutex mut_net_message_queue_;
  indigo::core::Vector<pb::GameServerMessage> net_message_queue_;
  std::mutex mut_pending_client_message_queue_;
  indigo::core::Vector<pb::GameClientSingleMessage>
      pending_client_message_queue_;

  // Simulation update helpers...
  system::LocomotionSystem locomotion_system_;
};

}  // namespace sanctify

#endif
