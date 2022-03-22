#include <igasset/igpack_loader.h>
#include <igasync/promise_combiner.h>
#include <igcore/log.h>
#include <pve_game_scene/ecs/utils.h>
#include <pve_game_scene/pve_game_scene.h>

using namespace sanctify;
using namespace indigo;
using namespace core;

namespace {
const char* kLogLabel = "PveGameScene";

const float kPingTime = 250.f / 1000.f;
const float kUnhealthyTime = 2.2f;
}  // namespace

std::shared_ptr<PveGameScene> PveGameScene::Create(
    std::shared_ptr<AppBase> app_base,
    indigo::core::Maybe<std::shared_ptr<NetClient>> net_client,
    std::shared_ptr<TaskList> main_thread_task_list,
    std::shared_ptr<TaskList> async_task_list) {
  auto ptr = std::shared_ptr<PveGameScene>(new PveGameScene(
      app_base, net_client, main_thread_task_list, async_task_list));

  ptr->kick_off_preload();
  ptr->kick_off_load();
  ptr->setup_depth_texture(app_base->Width, app_base->Height);

  if (net_client.has_value()) {
    auto& c = net_client.get();

    c->set_connection_state_changed_listener(
        [ptr](NetClient::ConnectionState state) {
          ptr->connection_state_ = state;
        });
    c->set_on_server_message_listener([ptr](pb::GameServerMessage msg) {
      if (ptr->client_stage_ == ClientStage::Preload) {
        // Discard messages while preloading - don't want to fill up the queue
        // too much!
        return;
      }

      // Discard messages that don't contain information we're interested in
      if (!msg.has_actions_list()) {
        return;
      }

      std::lock_guard<std::mutex> l(ptr->m_net_message_queue_);
      for (int i = 0; i < msg.actions_list().messages_size(); i++) {
        ptr->net_message_queue_.push_back(msg.actions_list().messages(i));
      }
    });
  }

  return ptr;
}

PveGameScene::PveGameScene(
    std::shared_ptr<AppBase> app_base,
    indigo::core::Maybe<std::shared_ptr<NetClient>> net_client,
    std::shared_ptr<TaskList> main_thread_task_list,
    std::shared_ptr<TaskList> async_task_list)
    : client_stage_(ClientStage::Preload),
      connection_state_(NetClient::ConnectionState::Disconnected),
      world_(),
      client_state_entity_(world_.create()),
      app_base_(app_base),
      main_thread_task_list_(main_thread_task_list),
      async_task_list_(async_task_list),
      net_client_(std::move(net_client)),
      on_preload_finish_(Promise<EmptyPromiseRsl>::create()),
      should_quit_(false),
      is_self_loaded_(false),
      client_time_(0.f),
      time_to_next_ping_(0.f),
      time_since_last_server_message_(0.f) {}

std::shared_ptr<indigo::core::Promise<indigo::core::EmptyPromiseRsl>>
PveGameScene::get_preload_finish_promise() const {
  return on_preload_finish_;
}

void PveGameScene::kick_off_preload() {
  // TODO (sessamekesh): load loading state resources
}

void PveGameScene::kick_off_load() {
  auto combiner = PromiseCombiner::Create();

  asset::IgpackLoader terrain_loader("resources/terrain-pve.igpack",
                                     async_task_list_);

  auto terrain_key = combiner->add(terrain_loader.extract_detour_navmesh(
                                       "pve-arena-navmesh", async_task_list_),
                                   async_task_list_);

  combiner->combine()->on_success(
      [this, l = shared_from_this(),
       terrain_key](const PromiseCombiner::PromiseCombinerResult& rsl) {
        asset::IgpackLoader::ExtractDetourNavmeshDataT terrain_rsl =
            rsl.move(terrain_key);

        if (terrain_rsl.is_right()) {
          Logger::err(kLogLabel)
              << "Failed to load Detour navmesh for PVE arena: "
              << asset::to_string(terrain_rsl.get_right());
          should_quit_ = true;
        }
        arena_navmesh_ = terrain_rsl.left_move();

        is_self_loaded_ = true;
      },
      main_thread_task_list_);
}

void PveGameScene::update(float dt) {
  // Update client, update server ping (if online) regardless of client stage
  client_time_ += dt;
  update_server_ping(dt);

  switch (client_stage_) {
    case ClientStage::Preload:
      return;
    case ClientStage::Loading:
      loading_update(dt);
      return;
    case ClientStage::Running:
      running_update(dt);
      return;
    case ClientStage::GameOver:
      game_over_update(dt);
      return;
  }

  Logger::err(kLogLabel) << "Unexpected [[update]] case hit: client stage is ("
                         << (int)client_stage_ << ")";
}

void PveGameScene::render() {
  switch (client_stage_) {
    case ClientStage::Preload:
      return;
    case ClientStage::Loading:
      loading_render();
      return;
    case ClientStage::Running:
      running_render();
      return;
    case ClientStage::GameOver:
      game_over_render();
      return;
  }

  Logger::err(kLogLabel) << "Unexpected [[render]] case hit: client stage is ("
                         << (int)client_stage_ << ")";
}

bool PveGameScene::should_quit() { return should_quit_; }

void PveGameScene::on_viewport_resize(uint32_t width, uint32_t height) {
  setup_depth_texture(width, height);
}

void PveGameScene::loading_update(float dt) {
  // Always update client sim time once loading has begun
  ecs::sim_clock_time(world_, client_state_entity_) += dt;

  // Netsync state...
  if (net_client_.has_value()) {
    sim_time_sync_system_.loading_update(world_, client_state_entity_, dt);
  }

  // Check loading state, possibly transition to "running"
  if (is_self_done_loading()) {
    client_stage_ = ClientStage::Running;
    running_update(dt);
    return;
  }
  // Loading...
}

void PveGameScene::loading_render() {
  // Loading...
}

void PveGameScene::running_update(float dt) {
  handle_update_netsync(dt);
  // Running...
}

void PveGameScene::running_render() {
  // Running...
}

void PveGameScene::game_over_update(float dt) {
  // Game over...
}

void PveGameScene::game_over_render() {
  // Game over...
}

void PveGameScene::handle_update_netsync(float dt) {
  if (!net_client_.has_value()) {
    return;
  }

  for (int i = 0; i < net_message_queue_.size(); i++) {
    const auto& msg = net_message_queue_[i];

    if (msg.has_game_snapshot_diff()) {
      ecs::cache_snapshot_diff(world_, client_state_entity_,
                               msg.game_snapshot_diff());
    } else if (msg.has_game_snapshot_full()) {
      ecs::cache_snapshot_full(world_, client_state_entity_,
                               msg.game_snapshot_full());
    } else if (msg.has_player_movement()) {
      ecs::add_player_movement_indicator(world_, msg.player_movement());
    } else if (msg.has_pong()) {
      if (client_stage_ == ClientStage::Loading) {
        sim_time_sync_system_.handle_pong(world_, client_state_entity_,
                                          msg.pong());
      }

      // TODO (sessamekesh): RTT stuff here (also use sim time sync system?)
    }
  }
}

void PveGameScene::update_server_ping(float dt) {
  if (net_client_.has_value()) {
    // TODO (sessamekesh): Move this to a health check system?
    //  Especially since this ping/pong will be involved in RTT calculation
    // Ping...
    time_to_next_ping_ -= dt;
    if (time_to_next_ping_ <= 0.f) {
      time_to_next_ping_ = ::kPingTime;

      pb::GameClientMessage msg{};
      auto* ping = msg.mutable_game_client_actions_list()
                       ->add_actions()
                       ->mutable_client_ping();
      ping->set_is_ready(is_self_loaded_);
      ping->set_local_time(ecs::sim_clock_time(world_, client_state_entity_));

      net_client_.get()->send_message(std::move(msg));
    }

    // Health check...
    time_since_last_server_message_ += dt;
  }
}

void PveGameScene::setup_depth_texture(uint32_t width, uint32_t height) {
  const wgpu::Device& device = app_base_->Device;
  depth_texture_ = iggpu::create_empty_texture_2d(
      device, width, height, 1, wgpu::TextureFormat::Depth24PlusStencil8,
      wgpu::TextureUsage::RenderAttachment);

  depth_view_ = depth_texture_.create_default_view();
}

bool PveGameScene::is_healthy() const {
  if (!net_client_.has_value()) {
    return true;
  }

  return time_since_last_server_message_ <= kUnhealthyTime;
}

bool PveGameScene::is_self_done_loading() {
  if (!is_self_loaded_) {
    return false;
  }

  if (net_client_.has_value()) {
    if (!sim_time_sync_system_.is_done_loading(world_, client_state_entity_)) {
      return false;
    }
  }

  return true;
}
