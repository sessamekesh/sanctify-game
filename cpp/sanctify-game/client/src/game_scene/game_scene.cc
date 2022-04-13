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
    std::shared_ptr<ViewportClickInput> viewport_click_info,
    TerrainShit terrain_shit,
    solid_animated::SolidAnimatedPipelineBuilder
        solid_animated_pipeline_builder,
    GameGeometryKeySet game_geometry_key_set, DebugGeoShit debug_geo_shit,
    SolidAnimatedGeoRegistry solid_animated_geo_registry,
    OzzSkeletonRegistry ozz_skeleton_registry,
    OzzAnimationRegistry ozz_animation_registry,
    std::shared_ptr<NetClient> net_client, float camera_movement_speed,
    float fovy) {
  auto game_scene = std::shared_ptr<GameScene>(new GameScene(
      base, arena_camera, camera_input_system, viewport_click_info,
      std::move(terrain_shit), std::move(solid_animated_pipeline_builder),
      game_geometry_key_set, std::move(debug_geo_shit),
      solid_animated_geo_registry, ozz_skeleton_registry,
      ozz_animation_registry, net_client, camera_movement_speed, fovy));

  game_scene->post_ctor_setup();

  return game_scene;
}

GameScene::GameScene(std::shared_ptr<AppBase> base, ArenaCamera arena_camera,
                     std::shared_ptr<IArenaCameraInput> camera_input_system,
                     std::shared_ptr<ViewportClickInput> viewport_click_info,
                     TerrainShit terrain_shit,
                     solid_animated::SolidAnimatedPipelineBuilder
                         solid_animated_pipeline_builder,
                     GameGeometryKeySet game_geometry_key_set,
                     DebugGeoShit debug_geo_shit,
                     SolidAnimatedGeoRegistry solid_animated_geo_registry,
                     OzzSkeletonRegistry ozz_skeleton_registry,
                     OzzAnimationRegistry ozz_animation_registry,
                     std::shared_ptr<NetClient> net_client,
                     float camera_movement_speed, float fovy)
    : base_(base),
      arena_camera_(arena_camera),
      arena_camera_input_(camera_input_system),
      viewport_click_info_(viewport_click_info),
      net_client_(net_client),
      terrain_shit_(std::move(terrain_shit)),
      camera_common_vs_ubo_(base_->Device),
      camera_common_fs_ubo_(base_->Device),
      common_lighting_ubo_(base_->Device),
      solid_animated_pipeline_builder_(
          std::move(solid_animated_pipeline_builder)),
      game_geometry_key_set_(game_geometry_key_set),
      solid_animated_geo_registry_(solid_animated_geo_registry),
      ozz_animation_registry_(ozz_animation_registry),
      ozz_skeleton_registry_(ozz_skeleton_registry),
      debug_geo_shit_(std::move(debug_geo_shit)),
      camera_movement_speed_(camera_movement_speed),
      fovy_(fovy),
      client_clock_(0.f),
      server_clock_(-1.f),
      connection_state_(net_client->get_connection_state()),
      snapshot_cache_(5),
      player_move_indicator_render_system_(
          glm::vec3(0.4f, 0.4f, 1.f), glm::vec3(0.f, 0.f, 0.6f), 0.4f,
          glm::vec3(1.f, 1.f, 1.f), glm::vec3(0.2f, 0.2f, 0.2f)) {}

void GameScene::post_ctor_setup() {
  arena_camera_input_->attach();
  viewport_click_info_->attach();

  setup_depth_texture(base_->Width, base_->Height);
  setup_pipelines(base_->preferred_swap_chain_texture_format());
  setup_render_systems();

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

GameScene::~GameScene() {
  arena_camera_input_->detach();
  viewport_click_info_->detach();
}

void GameScene::update(float dt) {
  if (server_clock_ >= 0.f) {
    server_clock_ += dt;
  }
  client_clock_ += dt;

  //
  // Server state changes
  //
  handle_server_events();
  reconcile_net_state();

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

  // Discard for now
  auto camera_events = arena_camera_input_->events_since_last_poll();

  auto maybe_click_evt = viewport_click_info_->get_frame_action(
      fovy_, (float)base_->Width / base_->Height,
      arena_camera_.look_at() - arena_camera_.position(),
      arena_camera_.position(), arena_camera_.screen_up());
  if (maybe_click_evt.has_value()) {
    pb::GameClientSingleMessage msg{};

    pb::Vec2* travel_request =
        msg.mutable_travel_to_location_request()->mutable_destination();

    glm::vec2 endpoint = maybe_click_evt.get().mapLocation;

    travel_request->set_x(endpoint.x);
    travel_request->set_y(endpoint.y);

    pending_client_message_queue_.push_back(msg);
  }

  //
  // Game logic execution
  //
  advance_simulation(world_, dt);

  //
  // Render state (not part of core game logic)
  //
  player_move_indicator_render_system_.update(dt);
  set_ozz_animation_keys_system_->update(world_, dt);

  //
  // Bookkeeping
  //
  destroy_children_system_.run(world_);

  //
  // Dispatch any queued up client messages
  //
  dispatch_client_messages();
}

void GameScene::render() {
  const wgpu::Device& device = base_->Device;

  // (this is the consolidated version that should be used for everybody)
  {
    auto& camera_vs_params = camera_common_vs_ubo_.get_mutable();
    auto& camera_fs_params = camera_common_fs_ubo_.get_mutable();

    camera_vs_params.matView = arena_camera_.mat_view();
    camera_vs_params.matProj = glm::perspective(
        fovy_, (float)base_->Width / base_->Height, 0.1f, 4000.f);
    camera_fs_params.cameraPos = arena_camera_.position();

    camera_common_vs_ubo_.sync(device);
    camera_common_fs_ubo_.sync(device);
  }

  //
  // ECS render system execution (pre-pass buffer updates)
  //
  attach_player_renderables_system_->run(world_);
  update_ozz_animation_buffers_system_->update_cpu_buffers(world_);
  update_ozz_animation_buffers_system_->update_gpu_buffers(
      world_, solid_animated_gpu_.pipeline, device);

  //
  // MAIN RENDER PASS execution
  //
  wgpu::CommandEncoder main_pass_command_encoder =
      device.CreateCommandEncoder();

  wgpu::RenderPassEncoder static_geo_pass =
      ::create_geo_pass(main_pass_command_encoder, depth_view_,
                        base_->SwapChain.GetCurrentTextureView(),
                        base_->preferred_swap_chain_texture_format());

  terrain_pipeline::RenderUtil(device, static_geo_pass, terrain_shit_.Pipeline)
      .set_frame_inputs(terrain_gpu_.frameInputs)
      .set_scene_inputs(terrain_gpu_.sceneInputs)
      .set_material_inputs(terrain_shit_.MaterialInputs)
      .set_instances(terrain_shit_.IdentityBuffer.InstanceBuffer,
                     terrain_shit_.IdentityBuffer.NumInstances)

      .set_geometry(terrain_shit_.BaseGeo)
      .draw()
      .set_geometry(terrain_shit_.DecorationGeo)
      .draw()
      .set_geometry(terrain_shit_.MidTowerGeo)
      .draw();

  const auto& debug_cube_instances = player_move_indicator_render_system_.get();
  debug_geo_shit_.cubeInstanceBuffer.update_index_data(device,
                                                       debug_cube_instances);

  debug_geo::RenderUtil(static_geo_pass, debug_geo_shit_.pipeline)
      .set_frame_inputs(debug_gpu_.frameInputs)
      .set_scene_inputs(debug_gpu_.sceneInputs)
      .set_geometry(debug_geo_shit_.cubeGeo)
      .set_instances(debug_geo_shit_.cubeInstanceBuffer)
      .draw();

  render_solid_renderables_system_->render(
      world_, device, static_geo_pass, solid_animated_gpu_.pipeline,
      solid_animated_gpu_.sceneInputs, solid_animated_gpu_.frameInputs);

  // TODO (sessamekesh): Run render systems for the player here!

  // TODO (sessamekesh): Update Emscripten library to support .End() method
#ifdef __EMSCRIPTEN__
  static_geo_pass.EndPass();
#else
  static_geo_pass.End();
#endif
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

void GameScene::setup_pipelines(wgpu::TextureFormat swap_chain_format) {
  const wgpu::Device& device = base_->Device;

  //
  // Solid animated
  //
  solid_animated_gpu_.pipeline =
      solid_animated_pipeline_builder_.create_pipeline(device,
                                                       swap_chain_format);
  solid_animated_gpu_.sceneInputs =
      solid_animated_gpu_.pipeline.create_scene_inputs(
          device, glm::normalize(glm::vec3(1.f, -3.f, 1.f)),
          glm::vec3(1.f, 1.f, 1.f), 0.3f, 50.f);
  solid_animated_gpu_.frameInputs =
      solid_animated_gpu_.pipeline.create_frame_inputs(
          device, camera_common_vs_ubo_, camera_common_fs_ubo_);

  solid_animated_material_registry_ = std::make_shared<
      ReadonlyResourceRegistry<solid_animated::MaterialPipelineInputs>>();
  ybot_materials_keys_.base = solid_animated_material_registry_->add_resource(
      solid_animated_gpu_.pipeline.create_material_inputs(
          device, glm::vec3(0.628f, 0.628f, 1.f)));
  ybot_materials_keys_.joint = solid_animated_material_registry_->add_resource(
      solid_animated_gpu_.pipeline.create_material_inputs(
          device, glm::vec3(0.1f, 0.1f, 0.2f)));

  //
  // Common
  //
  common_lighting_ubo_ = render::CommonLightingUbo(
      device, render::CommonLightingParamsData{
                  glm::normalize(glm::vec3(1.f, -3.f, 1.f)), 0.3f,
                  glm::vec3(1.f, 1.f, 1.f), 50.f});

  //
  // Terrain
  //
  terrain_gpu_.sceneInputs =
      terrain_shit_.Pipeline.create_scene_inputs(device, common_lighting_ubo_);
  terrain_gpu_.frameInputs = terrain_shit_.Pipeline.create_frame_inputs(
      device, camera_common_vs_ubo_, camera_common_fs_ubo_);

  debug_gpu_.sceneInputs = debug_geo_shit_.pipeline.create_scene_inputs(
      device, common_lighting_ubo_);
  debug_gpu_.frameInputs = debug_geo_shit_.pipeline.create_frame_inputs(
      device, camera_common_vs_ubo_, camera_common_fs_ubo_);

  // TODO (sessamekesh): Invalidate all materials that are stored in ECS!
}

void GameScene::setup_render_systems() {
  attach_player_renderables_system_ =
      std::make_shared<ecs::AttachPlayerRenderablesSystem>(
          ybot_materials_keys_.joint, ybot_materials_keys_.base,
          game_geometry_key_set_.ybotJointsGeoKey,
          game_geometry_key_set_.ybotBaseGeoKey);
  set_ozz_animation_keys_system_ =
      std::make_shared<ecs::SetOzzAnimationKeysSystem>(
          game_geometry_key_set_.ybotSkeletonKey,
          game_geometry_key_set_.ybotIdleAnimationKey,
          game_geometry_key_set_.ybotWalkAnimationKey);
  update_ozz_animation_buffers_system_ =
      std::make_shared<ecs::UpdateOzzAnimationBuffersSystem>(
          ozz_skeleton_registry_, ozz_animation_registry_,
          solid_animated_geo_registry_);
  render_solid_renderables_system_ =
      std::make_shared<ecs::RenderSolidRenderablesSystem>(
          solid_animated_geo_registry_, solid_animated_material_registry_);
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

  // Only consider the last 50 messages (if there are a lot of them!)k
  int start_idx = 0;
  if (messages.size() > 50) {
    start_idx = messages.size() - 50;
  }

  for (int i = start_idx; i < messages.size(); i++) {
    const pb::GameServerMessage& msg = messages[i];

    if (server_clock_ < 0.f) {
      server_clock_ = msg.clock_time();
    } else {
      server_clock_ = glm::min(server_clock_, msg.clock_time());
    }

    // Ignore any old messages (older than 8 seconds)
    if (msg.clock_time() < server_clock_ - 8.f) {
      continue;
    }

    if (msg.has_initial_connection_response()) {
      continue;
    } else if (msg.has_actions_list()) {
      for (int j = 0; j < msg.actions_list().messages_size(); j++) {
        handle_single_message(msg.actions_list().messages(j));
      }
    }
  }
}

void GameScene::handle_single_message(const pb::GameServerSingleMessage& msg) {
  // Common case first!
  if (msg.has_game_snapshot_diff()) {
    GameSnapshotDiff diff =
        GameSnapshotDiff::Deserialize(msg.game_snapshot_diff());
    Maybe<GameSnapshot> maybe_snapshot =
        snapshot_cache_.assemble_diff_and_store_snapshot(diff);
    if (maybe_snapshot.is_empty()) {
      // Ignore this snapshot, since it cannot be assembled with the current
      // state.
      return;
    }

    GameSnapshot snapshot = maybe_snapshot.move();
    reconcile_net_state_system_.register_server_snapshot(snapshot,
                                                         server_clock_);

    pb::GameClientSingleMessage msg{};
    msg.mutable_snapshot_received()->set_snapshot_id(snapshot.snapshot_id());
    pending_client_message_queue_.push_back(msg);

    return;
  }

  if (msg.has_game_snapshot_full()) {
    GameSnapshot snapshot = GameSnapshot::Deserialize(msg.game_snapshot_full());
    snapshot_cache_.store_server_snapshot(snapshot);
    reconcile_net_state_system_.register_server_snapshot(snapshot,
                                                         server_clock_);

    pb::GameClientSingleMessage msg{};
    msg.mutable_snapshot_received()->set_snapshot_id(snapshot.snapshot_id());
    pending_client_message_queue_.push_back(msg);

    return;
  }

  if (msg.has_player_movement()) {
    player_move_indicator_render_system_.add_at_location(
        glm::vec3(msg.player_movement().destination().x(), 0.f,
                  msg.player_movement().destination().y()));

    return;
  }

  Logger::err(kLogLabel) << "Unexpected single message type - has kind "
                         << (uint32_t)msg.msg_body_case()
                         << " (see sanctify-net.proto for what that is)";
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

void GameScene::reconcile_net_state() {
  reconcile_net_state_system_.advance_time_to_and_maybe_reconcile(
      world_,
      [this](entt::registry& server_sim, float dt) {
        advance_simulation(server_sim, dt);
      },
      server_clock_);
}

void GameScene::advance_simulation(entt::registry& world, float dt) {
  const float kMaxFrameTime = 1.f / 60.f;

  while (dt > kMaxFrameTime) {
    locomotion_system_.apply_standard_locomotion(world, kMaxFrameTime);
    dt -= kMaxFrameTime;
  }
  locomotion_system_.apply_standard_locomotion(world, dt);
}
