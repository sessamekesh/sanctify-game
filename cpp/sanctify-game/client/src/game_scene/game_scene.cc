#include <game_scene/game_scene.h>
#include <iggpu/util.h>
#include <io/glfw_event_emitter.h>

using namespace indigo;
using namespace core;
using namespace sanctify;

namespace {
const char* kLogLabel = "GameScene";

wgpu::RenderPassEncoder create_geo_pass(
    wgpu::CommandEncoder main_pass_command_encoder,
    const wgpu::TextureView& depth_view, wgpu::TextureView backbuffer_view,
    wgpu::TextureFormat swap_chain_format) {
  wgpu::RenderPassDepthStencilAttachment depth_attachment{};
  depth_attachment.clearDepth = 1.f;
  depth_attachment.clearStencil = 0x00;
  depth_attachment.depthLoadOp = wgpu::LoadOp::Clear;
  depth_attachment.depthStoreOp = wgpu::StoreOp::Store;
  depth_attachment.stencilLoadOp = wgpu::LoadOp::Clear;
  depth_attachment.stencilStoreOp = wgpu::StoreOp::Store;
  depth_attachment.view = depth_view;

  wgpu::RenderPassColorAttachment color_attachment{};
  color_attachment.clearColor = {0.1f, 0.1f, 0.11f, 1.f};
  color_attachment.loadOp = wgpu::LoadOp::Clear;
  color_attachment.storeOp = wgpu::StoreOp::Store;
  color_attachment.view = backbuffer_view;

  wgpu::RenderPassDescriptor desc{};
  desc.colorAttachments = &color_attachment;
  desc.colorAttachmentCount = 1;
  desc.depthStencilAttachment = &depth_attachment;

  return main_pass_command_encoder.BeginRenderPass(&desc);
}
}  // namespace

GameScene::GameScene(std::shared_ptr<AppBase> base, ArenaCamera arena_camera,
                     std::shared_ptr<IArenaCameraInput> camera_input_system,
                     TerrainShit terrain_shit, PlayerShit player_shit,
                     float camera_movement_speed, float fovy)
    : base_(base),
      arena_camera_(arena_camera),
      arena_camera_input_(camera_input_system),
      terrain_shit_(std::move(terrain_shit)),
      player_shit_(std::move(player_shit)),
      camera_movement_speed_(camera_movement_speed),
      fovy_(fovy) {
  arena_camera_input_->attach();

  setup_depth_texture(base->Width, base->Height);
}

GameScene::~GameScene() { arena_camera_input_->detach(); }

void GameScene::update(float dt) {
  ArenaCameraInputState input_state = arena_camera_input_->get_input_state();

  if (input_state.ScreenRightMovement != 0.f ||
      input_state.ScreenUpMovement != 0.f) {
    glm::vec3 camera_movement =
        (arena_camera_.screen_right() * input_state.ScreenRightMovement +
         arena_camera_.screen_up() * input_state.ScreenUpMovement) *
        camera_movement_speed_;
    arena_camera_.set_look_at(arena_camera_.look_at() + camera_movement);
  }

  // TODO (sessamekesh): Also handle camera events (click and drag) here
}

void GameScene::render() {
  const wgpu::Device& device = base_->Device;

  terrain_shit_.FrameInputs.CameraFragmentParamsUbo.get_mutable().CameraPos =
      arena_camera_.position();
  terrain_shit_.FrameInputs.CameraFragmentParamsUbo.sync(device);

  // TODO (sessamekesh): Consolidate
  {
    auto& camera_params =
        terrain_shit_.FrameInputs.CameraParamsUbo.get_mutable();
    camera_params.MatView = arena_camera_.mat_view();
    camera_params.MatProj = glm::perspective(
        fovy_, (float)base_->Width / base_->Height, 0.1f, 4000.f);
    terrain_shit_.FrameInputs.CameraParamsUbo.sync(device);
  }
  {
    auto& camera_params =
        player_shit_.FrameInputs.CameraParamsUbo.get_mutable();
    camera_params.MatView = arena_camera_.mat_view();
    camera_params.MatProj = glm::perspective(
        fovy_, (float)base_->Width / base_->Height, 0.1f, 4000.f);
    player_shit_.FrameInputs.CameraParamsUbo.sync(device);
  }

  //
  // Encode commands
  //
  wgpu::CommandEncoder main_pass_command_encoder =
      device.CreateCommandEncoder();

  wgpu::RenderPassEncoder static_geo_pass =
      ::create_geo_pass(main_pass_command_encoder, depth_view_,
                        base_->SwapChain.GetCurrentTextureView(),
                        base_->preferred_swap_chain_texture_format());

  terrain_pipeline::RenderUtil(device, static_geo_pass, terrain_shit_.Pipeline)
      .set_frame_inputs(terrain_shit_.FrameInputs)
      .set_material_inputs(terrain_shit_.MaterialInputs)
      .set_scene_inputs(terrain_shit_.SceneInputs)
      .set_instances(terrain_shit_.IdentityBuffer.InstanceBuffer,
                     terrain_shit_.IdentityBuffer.NumInstances)
      .set_geometry(terrain_shit_.BaseGeo)
      .draw()
      .set_geometry(terrain_shit_.DecorationGeo)
      .draw();

  solid_animated::RenderUtil(static_geo_pass, player_shit_.Pipeline)
      .set_frame_inputs(player_shit_.FrameInputs)
      .set_scene_inputs(player_shit_.SceneInputs)
      .set_instances(player_shit_.InstanceBuffers)
      .set_geometry(player_shit_.BaseGeo)
      .set_material_inputs(player_shit_.BaseMaterial)
      .draw()
      .set_geometry(player_shit_.JointsGeo)
      .set_material_inputs(player_shit_.JointsMaterial)
      .draw();

  static_geo_pass.EndPass();
  wgpu::CommandBuffer main_pass_commands = main_pass_command_encoder.Finish();

  device.GetQueue().Submit(1, &main_pass_commands);

#ifndef __EMSCRIPTEN__
  base_->SwapChain.Present();
#endif
}

bool GameScene::should_quit() { return false; }

void GameScene::on_viewport_resize(uint32_t width, uint32_t height) {
  setup_depth_texture(width, height);
}

void GameScene::setup_depth_texture(uint32_t width, uint32_t height) {
  const wgpu::Device& device = base_->Device;
  depth_texture_ = iggpu::create_empty_texture_2d(
      device, width, height, 1, wgpu::TextureFormat::Depth24PlusStencil8,
      wgpu::TextureUsage::RenderAttachment);

  wgpu::TextureViewDescriptor depth_view_desc =
      iggpu::view_desc_of(depth_texture_);
  depth_view_ = depth_texture_.GpuTexture.CreateView(&depth_view_desc);
}