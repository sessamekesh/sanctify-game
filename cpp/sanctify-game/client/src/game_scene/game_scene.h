#ifndef SANCTIFY_GAME_CLIENT_SRC_GAME_SCENE_GAME_SCENE_H
#define SANCTIFY_GAME_CLIENT_SRC_GAME_SCENE_GAME_SCENE_H

#include <iggpu/texture.h>
#include <io/arena_camera_controller/arena_camera_input.h>
#include <render/camera/arena_camera.h>
#include <render/solid_animated/solid_animated_pipeline.h>
#include <render/terrain/terrain_geo.h>
#include <render/terrain/terrain_pipeline.h>
#include <scene_base.h>
#include <webgpu/webgpu_cpp.h>

namespace sanctify {

class GameScene : public ISceneBase {
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

 public:
  GameScene(std::shared_ptr<AppBase> base, ArenaCamera arena_camera,
            std::shared_ptr<IArenaCameraInput> camera_input_system,
            TerrainShit terrain_shit, PlayerShit player_shit,
            float camera_movement_speed, float fovy);
  ~GameScene();

  // ISceneBase
  void update(float dt) override;
  void render() override;
  bool should_quit() override;
  void on_viewport_resize(uint32_t width, uint32_t height) override;

 private:
  void setup_depth_texture(uint32_t width, uint32_t height);

 private:
  std::shared_ptr<AppBase> base_;

  indigo::iggpu::Texture depth_texture_;
  wgpu::TextureView depth_view_;

  std::shared_ptr<IArenaCameraInput> arena_camera_input_;
  float camera_movement_speed_;
  float fovy_;
  ArenaCamera arena_camera_;

  // Random shit that should be replaced with better, more fleshed out systems
  // You'll need a scene graph and renderable handle system, especially when
  // dealing with Lua, and might as well put in some simple AABB/frustum culling
  TerrainShit terrain_shit_;
  PlayerShit player_shit_;
};

}  // namespace sanctify

#endif