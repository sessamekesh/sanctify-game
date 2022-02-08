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

std::shared_ptr<GameScene> GameScene::Create(
    std::shared_ptr<AppBase> base, ArenaCamera arena_camera,
    std::shared_ptr<IArenaCameraInput> camera_input_system,
    TerrainShit terrain_shit, PlayerShit player_shit,
    std::shared_ptr<NetClient> net_client, float camera_movement_speed,
    float fovy) {
  auto game_scene = std::shared_ptr<GameScene>(new GameScene(
      base, arena_camera, camera_input_system, std::move(terrain_shit),
      std::move(player_shit), net_client, camera_movement_speed, fovy));

  game_scene->post_ctor_setup();

  return game_scene;
}

GameScene::GameScene(std::shared_ptr<AppBase> base, ArenaCamera arena_camera,
                     std::shared_ptr<IArenaCameraInput> camera_input_system,
                     TerrainShit terrain_shit, PlayerShit player_shit,
                     std::shared_ptr<NetClient> net_client,
                     float camera_movement_speed, float fovy)
    : base_(base),
      arena_camera_(arena_camera),
      arena_camera_input_(camera_input_system),
      net_client_(net_client),
      terrain_shit_(std::move(terrain_shit)),
      player_shit_(std::move(player_shit)),
      camera_movement_speed_(camera_movement_speed),
      fovy_(fovy),
      client_clock_(0.f),
      server_clock_(-1.f),
      connection_state_(net_client->get_connection_state()) {}

void GameScene::post_ctor_setup() {
  arena_camera_input_->attach();

  setup_depth_texture(base_->Width, base_->Height);

  auto that = weak_from_this();
  net_client_->set_connection_state_changed_listener(
      [that](NetClient::ConnectionState state) {
        std::shared_ptr<GameScene> game_scene = that.lock();
        if (!game_scene) {
          return;
        }

        std::lock_guard<std::mutex> l(game_scene->mut_connection_state_);

        if (game_scene->connection_state_ != state) {
          Logger::log(kLogLabel)
              << "Player connection state changed to " << to_string(state);
        }

        game_scene->connection_state_ = state;

        // TODO (sessamekesh): Queue up a net event instead (e.g., for
        // disconnected)
      });
  net_client_->set_on_server_message_listener(
      [that](pb::GameServerMessage msg) {
        std::shared_ptr<GameScene> game_scene = that.lock();
        if (!game_scene) {
          return;
        }

        std::lock_guard<std::mutex> l(game_scene->mut_net_message_queue_);
        game_scene->net_message_queue_.push_back(std::move(msg));
      });
}

GameScene::~GameScene() { arena_camera_input_->detach(); }

void GameScene::update(float dt) {
  if (server_clock_ >= 0.f) {
    server_clock_ += dt;
  }
  client_clock_ += dt;

  //
  // Server state changes
  //
  handle_server_events();

  //
  // User input
  //
  ArenaCameraInputState input_state = arena_camera_input_->get_input_state();

  if (input_state.ScreenRightMovement != 0.f ||
      input_state.ScreenUpMovement != 0.f) {
    glm::vec3 camera_movement =
        (arena_camera_.screen_right() * input_state.ScreenRightMovement +
         arena_camera_.screen_up() * input_state.ScreenUpMovement) *
        camera_movement_speed_;
    arena_camera_.set_look_at(arena_camera_.look_at() + camera_movement);
  }

  // TODO (sessamekesh): Handle player clicks here!

  // TODO (sessamekesh): Also handle camera events (click and drag) here

  //
  // Game logic execution
  //

  //
  // Dispatch any queued up client messages
  //
  dispatch_client_messages();
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

  auto player_instance_data = player_render_system_.get_instance_data(world_);
  player_shit_.InstanceBuffers.update_index_data(device, player_instance_data);

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

void GameScene::handle_server_events() {
  Vector<pb::GameServerMessage> messages;
  {
    std::lock_guard l(mut_net_message_queue_);
    // Should never be the case, but worth a go anyways.
    if (net_message_queue_.size() == 0u) {
      return;
    }

    // TODO (sessamekesh): this allocation could be avoided by double-buffering
    // network message queues - switch out the receiver and which is being
    // processed at this point.
    messages = std::move(net_message_queue_);
    ::new (&net_message_queue_) Vector<pb::GameServerMessage>(4);
  }

  for (int i = 0; i < messages.size(); i++) {
    const pb::GameServerMessage& msg = messages[i];

    if (server_clock_ < 0.f) {
      server_clock_ = msg.clock_time();
    } else {
      server_clock_ = glm::min(server_clock_, msg.clock_time());
    }

    if (msg.has_initial_connection_response()) {
      continue;
    } else if (msg.has_actions_list()) {
      for (int j = 0; j < msg.actions_list().messages_size(); j++) {
        const pb::GameServerSingleMessage& single_msg =
            msg.actions_list().messages(j);
      }
    }
  }
}

void GameScene::dispatch_client_messages() {
  std::lock_guard<std::mutex> l(mut_pending_client_message_queue_);

  if (pending_client_message_queue_.size() == 0) {
    // Unlikely but possible
    return;
  }

  // TODO (sessamekesh): handle message chunking here (only send up to ~8kb of
  // data or whatever)
  pb::GameClientMessage msg{};
  pb::GameClientActionsList* actions_list =
      msg.mutable_game_client_actions_list();
  for (int i = 0; i < pending_client_message_queue_.size(); i++) {
    pb::GameClientSingleMessage* action = actions_list->add_actions();
    *action = std::move(pending_client_message_queue_[i]);
  }

  ::new (&pending_client_message_queue_) Vector<pb::GameClientSingleMessage>(4);

  net_client_->send_message(msg);
}