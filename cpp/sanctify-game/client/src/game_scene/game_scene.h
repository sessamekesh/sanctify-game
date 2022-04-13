#ifndef SANCTIFY_GAME_CLIENT_SRC_GAME_SCENE_GAME_SCENE_H
#define SANCTIFY_GAME_CLIENT_SRC_GAME_SCENE_GAME_SCENE_H

#include <ecs/systems/attach_player_renderables.h>
#include <ecs/systems/destroy_children_system.h>
#include <ecs/systems/solid_animation_systems.h>
#include <game_scene/systems/player_move_indicator_render_system.h>
#include <igcore/vector.h>
#include <iggpu/texture.h>
#include <io/arena_camera_controller/arena_camera_input.h>
#include <io/viewport_click/viewport_click_controller_input.h>
#include <net/reconcile_net_state_system.h>
#include <net/snapshot_cache.h>
#include <netclient/net_client.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/skeleton.h>
#include <render/camera/arena_camera.h>
#include <render/common/camera_ubo.h>
#include <render/debug_geo/debug_geo.h>
#include <render/debug_geo/debug_geo_pipeline.h>
#include <render/solid_animated/solid_animated_pipeline.h>
#include <render/terrain/terrain_geo.h>
#include <render/terrain/terrain_pipeline.h>
#include <sanctify-game-common/gameplay/locomotion.h>
#include <scene_base.h>
#include <util/registry_types.h>
#include <util/resource_registry.h>
#include <webgpu/webgpu_cpp.h>

#include <entt/entt.hpp>

namespace sanctify {

class GameScene : public ISceneBase,
                  public std::enable_shared_from_this<GameScene> {
 public:
  struct TerrainShit {
    terrain_pipeline::TerrainPipelineBuilder PipelineBuilder;
    terrain_pipeline::TerrainPipeline Pipeline;
    terrain_pipeline::MaterialPipelineInputs MaterialInputs;
    terrain_pipeline::TerrainGeo BaseGeo;
    terrain_pipeline::TerrainGeo DecorationGeo;
    terrain_pipeline::TerrainGeo MidTowerGeo;
    terrain_pipeline::TerrainMatWorldInstanceBuffer IdentityBuffer;
  };

  // TODO (sessamekesh): Consolidate identical bind groups even more!!!
  //  You'll probably need to stop using the UboBase concept for these
  //  shared buffers like camera common and lighting common.
  struct DebugGeoShit {
    debug_geo::DebugGeoPipelineBuilder pipelineBuilder;
    debug_geo::DebugGeoPipeline pipeline;

    debug_geo::DebugGeo cubeGeo;
    debug_geo::InstanceBuffer cubeInstanceBuffer;
  };

  struct GameGeometryKeySet {
    ReadonlyResourceRegistry<ozz::animation::Skeleton>::Key ybotSkeletonKey;
    ReadonlyResourceRegistry<ozz::animation::Animation>::Key
        ybotIdleAnimationKey;
    ReadonlyResourceRegistry<ozz::animation::Animation>::Key
        ybotWalkAnimationKey;
    ReadonlyResourceRegistry<solid_animated::SolidAnimatedGeo>::Key
        ybotJointsGeoKey;
    ReadonlyResourceRegistry<solid_animated::SolidAnimatedGeo>::Key
        ybotBaseGeoKey;
  };

 public:
  static std::shared_ptr<GameScene> Create(
      std::shared_ptr<AppBase> base, ArenaCamera arena_camera,
      std::shared_ptr<IArenaCameraInput> camera_input_system,
      std::shared_ptr<ViewportClickInput> viewport_click_info,
      TerrainShit terrain_shit,
      solid_animated::SolidAnimatedPipelineBuilder
          solid_animated_pipeline_builder,
      GameGeometryKeySet game_geometry_key_set, DebugGeoShit debug_geo_shit,
      SolidAnimatedGeoRegistry solid_animated_geo_registry,
      OzzSkeletonRegistry ozz_skeleton_registry,
      OzzAnimationRegistry ozz_animation_registry,
      std::shared_ptr<NetClient> net_client, float camera_movement_speed,
      float fovy);
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
            TerrainShit terrain_shit,
            solid_animated::SolidAnimatedPipelineBuilder
                solid_animated_pipeline_builder,
            GameGeometryKeySet game_geometry_key_set,
            DebugGeoShit debug_geo_shit,
            SolidAnimatedGeoRegistry solid_animated_geo_registry,
            OzzSkeletonRegistry ozz_animation_registry,
            OzzAnimationRegistry ozz_skeleton_registry,
            std::shared_ptr<NetClient> net_client, float camera_movement_speed,
            float fovy);
  void post_ctor_setup();

  void setup_depth_texture(uint32_t width, uint32_t height);
  void setup_pipelines(wgpu::TextureFormat swap_chain_format);
  void setup_render_systems();

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
  DebugGeoShit debug_geo_shit_;

  // Hey, here's some better, more fleshed-out systems...
  render::CameraCommonVsUbo camera_common_vs_ubo_;
  render::CameraCommonFsUbo camera_common_fs_ubo_;
  render::CommonLightingUbo common_lighting_ubo_;
  solid_animated::SolidAnimatedPipelineBuilder solid_animated_pipeline_builder_;
  SolidAnimatedGeoRegistry solid_animated_geo_registry_;
  SolidAnimatedMaterialRegistry solid_animated_material_registry_;
  OzzSkeletonRegistry ozz_skeleton_registry_;
  OzzAnimationRegistry ozz_animation_registry_;

  // Eeeeh I still don't like this part
  GameGeometryKeySet game_geometry_key_set_;
  struct {
    ReadonlyResourceRegistry<solid_animated::MaterialPipelineInputs>::Key base;
    ReadonlyResourceRegistry<solid_animated::MaterialPipelineInputs>::Key joint;
  } ybot_materials_keys_;

  // ... and the resources that are created in ctor from pipeline things
  struct {
    solid_animated::SolidAnimatedPipeline pipeline;
    solid_animated::ScenePipelineInputs sceneInputs;
    solid_animated::FramePipelineInputs frameInputs;
  } solid_animated_gpu_;
  struct {
    terrain_pipeline::ScenePipelineInputs sceneInputs;
    terrain_pipeline::FramePipelineInputs frameInputs;
  } terrain_gpu_;
  struct {
    debug_geo::ScenePipelineInputs sceneInputs;
    debug_geo::FramePipelineInputs frameInputs;
  } debug_gpu_;

  // Net client (communicate with server)
  // TODO (sessamekesh): The game should be able to run with or without a net
  //  client! Run full local sim if the net client is not present (different
  //  impl?)
  std::shared_ptr<NetClient> net_client_;

  // Game state...
  entt::registry world_;
  float client_clock_;

  // Net state...
  float server_clock_;
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
  std::shared_ptr<ecs::AttachPlayerRenderablesSystem>
      attach_player_renderables_system_;
  ecs::DestroyChildrenSystem destroy_children_system_;
  std::shared_ptr<ecs::SetOzzAnimationKeysSystem>
      set_ozz_animation_keys_system_;
  std::shared_ptr<ecs::UpdateOzzAnimationBuffersSystem>
      update_ozz_animation_buffers_system_;
  std::shared_ptr<ecs::RenderSolidRenderablesSystem>
      render_solid_renderables_system_;
  system::LocomotionSystem locomotion_system_;
};

}  // namespace sanctify

#endif
