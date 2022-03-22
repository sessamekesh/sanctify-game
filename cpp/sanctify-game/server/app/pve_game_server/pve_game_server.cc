#include <app/pve_game_server/ecs/player_connect_state.h>
#include <app/pve_game_server/pve_game_server.h>
#include <igasset/igpack_loader.h>
#include <igasync/promise_combiner.h>
#include <sanctify-game-common/gameplay/locomotion_components.h>
#include <sanctify-game-common/gameplay/net_sync_components.h>
#include <sanctify-game-common/gameplay/player_definition_components.h>

using namespace sanctify;
using namespace indigo;
using namespace core;

using hrclock = std::chrono::high_resolution_clock;
using namespace std::chrono_literals;

namespace {
const char* kLogLabel = "PveGameServer";

// Maximum number of expected players (this will eventually be moved into a
//  configuration thing)
const int kMaxPlayers = 5;

std::string to_string(PveGameServer::ServerStage stage) {
  switch (stage) {
    case PveGameServer::ServerStage::Initializing:
      return "Initializing";
    case PveGameServer::ServerStage::WaitingForPlayers:
      return "WaitingForPlayers";
    case PveGameServer::ServerStage::Running:
      return "Running";
    case PveGameServer::ServerStage::GameOver:
      return "GameOver";
    case PveGameServer::ServerStage::Terminated:
      return "Terminated";
  }
}

void attach_player_connect_state(entt::registry& world, entt::entity e,
                                 PlayerId player_id) {
  world.emplace<PlayerConnectStateComponent>(
      e, player_id, PlayerConnectState::Disconnected,
      PlayerConnectionType::None, PlayerReadyState::NotReady);
  world.emplace<QueuedMessagesComponent>(e);
}

void attach_message(entt::registry& world, entt::entity e,
                    pb::GameServerSingleMessage msg,
                    bool attach_if_unhealthy = false) {
  if (!world.valid(e)) return;
  auto& queued_messages = world.get<QueuedMessagesComponent>(e);

  // Do not queue messages if the connection is not present...
  auto& conn_state = world.get<PlayerConnectStateComponent>(e);
  if (conn_state.connectState == PlayerConnectState::Disconnected) {
    return;
  }

  // ... or if its unhealthy, and we don't explicitly specify that we want
  //  to send the message anyways...
  if (conn_state.connectState == PlayerConnectState::Unhealthy &&
      !attach_if_unhealthy) {
    return;
  }

  queued_messages.unsentMessages.push_back(msg);
}

}  // namespace

std::shared_ptr<PveGameServer> PveGameServer::Create(
    std::shared_ptr<TaskList> async_task_list, bool use_fixed_timestamp) {
  auto server = std::shared_ptr<PveGameServer>(
      new PveGameServer(async_task_list, use_fixed_timestamp));
  server->initialize();
  return server;
}

PveGameServer::PveGameServer(std::shared_ptr<TaskList> async_task_list,
                             bool use_fixed_timestamp)
    : use_fixed_timestamp_(use_fixed_timestamp),
      server_stage_(ServerStage::Initializing),
      main_thread_task_list_(std::make_shared<TaskList>()),
      async_task_list_(async_task_list),
      player_cb_set_(Promise<EmptyPromiseRsl>::create()),
      shutdown_promise_(Promise<EmptyPromiseRsl>::create()),
      player_message_cb_(nullptr),
      net_event_organizer_(::kMaxPlayers),
      is_running_(false),
      sim_clock_(0.f),
      next_net_sync_id_(1u),
      game_thread_([this]() {
        Logger::log(kLogLabel)
            << "Beginning simulation on thread " << std::this_thread::get_id();
        is_running_ = true;

        auto last_frame = hrclock::now();
        std::this_thread::sleep_for(2ms);
        while (is_running_) {
          auto this_frame = hrclock::now();

          using FpSeconds =
              std::chrono::duration<float, std::chrono::seconds::period>;
          float dt = FpSeconds(this_frame - last_frame).count();
          last_frame = this_frame;

          if (use_fixed_timestamp_) {
            dt = 8.f / 1000.f;
          }

          this->update(dt);

          // Wait for the next game tick (8ms - make this configurable)
          // Execute main thread tasks as long as there are tasks to execute,
          //  and then delay until the next tick when tasks are exhausted.
          auto next_tick = this_frame + 8ms;
          while (hrclock::now() < next_tick &&
                 main_thread_task_list_->execute_next()) {
          }
          std::this_thread::sleep_until(next_tick);
        }
      }) {}

void PveGameServer::initialize() {
  auto combiner = PromiseCombiner::Create();

  // TODO (sessamekesh): Get the list of actual players that are expected from
  //  API or initialization or whatever. This will do for now.
  uint64_t str_hash = (uint64_t)std::hash<std::string>{}("test-token");
  expected_players_.push_back(PlayerId{str_hash});

  combiner->add(player_cb_set_, async_task_list_);

  asset::IgpackLoader loader("resources/terrain-pve.igpack", async_task_list_);

  auto navmesh_key = combiner->add(
      loader.extract_detour_navmesh("pve-arena-navmesh", async_task_list_),
      async_task_list_);

  combiner->combine()->on_success(
      [this, navmesh_key](const PromiseCombiner::PromiseCombinerResult& rsl) {
        if (server_stage_ != ServerStage::Initializing) {
          Logger::err(kLogLabel) << "Server stage was already set as "
                                 << ::to_string(server_stage_)
                                 << " at the end of initialize phase!";
          return;
        }

        asset::IgpackLoader::ExtractDetourNavmeshDataT navmesh =
            rsl.move(navmesh_key);

        if (navmesh.is_right()) {
          Logger::err(kLogLabel)
              << "Server startup failed because navmesh could not be loaded: "
              << asset::to_string(navmesh.get_right());
          server_stage_ = ServerStage::Terminated;
          is_running_ = false;
          return;
        }

        navmesh_ = navmesh.left_move();

        server_stage_ = ServerStage::WaitingForPlayers;
      },
      main_thread_task_list_);
}

void PveGameServer::receive_message_for_player(
    PlayerId player_id, sanctify::pb::GameClientMessage client_msg) {
  switch (server_stage_) {
    case ServerStage::Initializing:
      Logger::log(kLogLabel) << "Ignoring message received during "
                                "'Initializing' phase from player "
                             << player_id.Id;
      return;

    case ServerStage::Terminated:
      Logger::log(kLogLabel)
          << "Message received during 'Terminated' phase from player "
          << player_id.Id;
      return;

    case ServerStage::WaitingForPlayers:
    case ServerStage::Running:
    case ServerStage::GameOver:
      net_event_organizer_.recv_message(player_id, std::move(client_msg));
      return;
  }
}

void PveGameServer::set_player_message_receiver(PlayerMessageCb cb) {
  player_message_cb_ = cb;
  if (!player_cb_set_->is_finished()) {
    player_cb_set_->resolve({});
  }
}

void PveGameServer::set_netstate(
    const PlayerId& player_id,
    NetServer::PlayerConnectionState connection_state) {
  switch (server_stage_) {
    case ServerStage::Initializing:
      Logger::log(kLogLabel)
          << "Ignoring player netstate update received during "
             "'Initializing' phase from player "
          << player_id.Id;
      return;

    case ServerStage::Terminated:
      Logger::log(kLogLabel)
          << "Player netstate updated during 'Terminated' phase from player "
          << player_id.Id;
      return;

    case ServerStage::WaitingForPlayers:
    case ServerStage::Running:
    case ServerStage::GameOver:
      net_event_organizer_.set_net_server_state(player_id, connection_state);
      return;
  }
}

std::shared_ptr<Promise<bool>> PveGameServer::try_connect_player(
    const PlayerId& pid) {
  switch (server_stage_) {
    case ServerStage::Initializing:
    case ServerStage::Terminated:
      // Do not accept player connections before server is ready or after it's
      //  finished acting as a game server
      return Promise<bool>::immediate(false);

    default:
      // Fallthrough
      break;
  }

  if (!net_event_organizer_.add_player(pid)) {
    return Promise<bool>::immediate(false);
  }

  std::lock_guard<std::mutex> l(m_unhandled_connects_);
  auto promise = Promise<bool>::create();
  unhandled_connects_.push_back({pid, promise});
  return promise;
}

std::shared_ptr<Promise<EmptyPromiseRsl>> PveGameServer::shutdown() {
  return shutdown_promise_;
}

void PveGameServer::update(float dt) {
  switch (server_stage_) {
    case ServerStage::Initializing:
    case ServerStage::Terminated:
      return;

    case ServerStage::WaitingForPlayers:
      update_waiting_for_players(dt);
      break;

    case ServerStage::Running:
      update_running(dt);
      break;

    case ServerStage::GameOver:
      update_game_over(dt);
      break;
  }

  if (player_message_cb_ != nullptr) {
    send_client_messages_system_.update(world_, player_message_cb_);
  }
}

void PveGameServer::update_waiting_for_players(float dt) {
  sim_clock_ += dt;

  for (const auto& player : connected_player_entities_) {
    PlayerId pid = std::get<PlayerId>(player);
    entt::entity e = std::get<entt::entity>(player);
    if (!world_.valid(e)) {
      continue;
    }

    maybe_queue_pong(pid, e);
  }

  bool all_ready = true;
  for (const auto& expected_player : expected_players_) {
    all_ready = all_ready && net_event_organizer_.ready_state(expected_player);
  }

  if (all_ready) {
    create_initial_game_scene();
    server_stage_ = ServerStage::Running;
    return;
  }
}

void PveGameServer::update_running(float dt) {
  sim_clock_ += dt;

  // Handle player input events...
  for (auto& player : connected_player_entities_) {
    PlayerId pid = std::get<PlayerId>(player);
    entt::entity e = std::get<entt::entity>(player);
    if (!world_.valid(e)) {
      continue;
    }

    maybe_queue_pong(pid, e);

    auto movement_evt =
        net_event_organizer_.get_move_command(std::get<PlayerId>(player));
    if (movement_evt.has_value()) {
      component::PlayerNavRequestComponent::attach_on(world_, e,
                                                      movement_evt.get());
    }

    auto last_snapshot_recvd =
        net_event_organizer_.last_acked_snapshot_id(std::get<PlayerId>(player));
    if (last_snapshot_recvd.has_value()) {
      auto* net_state = world_.try_get<PlayerNetStateComponent>(e);
      auto* conn_state = world_.try_get<PlayerConnectStateComponent>(e);
      if (conn_state != nullptr && net_state != nullptr &&
          conn_state->connectState == PlayerConnectState::Healthy &&
          net_state->lastAckedSnapshotId < last_snapshot_recvd.get()) {
        net_state->lastAckedSnapshotId = last_snapshot_recvd.get();
      }
    }
  }

  // Handle server logic (including sending out messages to clients!)
  net_state_update_system_.update(world_, net_event_organizer_);
  player_nav_system_.update(world_, navmesh_.get());
  locomotion_system_.apply_standard_locomotion(world_, dt);
  queue_client_messages_system_.update(world_, sim_clock_);
}

void PveGameServer::update_game_over(float dt) { sim_clock_ += dt; }

void PveGameServer::maybe_queue_pong(const PlayerId& pid, entt::entity e) {
  auto ping_record_opt = net_event_organizer_.ping_record(pid);
  if (ping_record_opt.is_empty()) {
    return;
  }
  auto ping_record = ping_record_opt.move();

  // TODO (sessamekesh): telemetry against the ping

  pb::GameServerSingleMessage msg;
  auto* pong_msg = msg.mutable_pong();
  pong_msg->set_sim_time(sim_clock_);
  if (ping_record.pingId.has_value()) {
    pong_msg->set_ping_id(ping_record.pingId.get());
  }
  ::attach_message(world_, e, std::move(msg));
}

std::vector<std::pair<PlayerId, entt::entity>>
PveGameServer::get_new_players() {
  // Handle new player joins...
  std::vector<std::pair<PlayerId, entt::entity>> new_players;
  {
    std::lock_guard<std::mutex> l(m_unhandled_connects_);
    for (auto& unhandled_connect : unhandled_connects_) {
      if (connected_player_entities_.find_l(unhandled_connect.first) ==
          connected_player_entities_.end()) {
        auto e = world_.create();
        ::attach_player_connect_state(world_, e, unhandled_connect.first);

        connected_player_entities_.insert(unhandled_connect.first, e);
        unhandled_connect.second->resolve(true);
        new_players.push_back({unhandled_connect.first, e});
      }
    }
  }

  return new_players;
}

void PveGameServer::create_initial_game_scene() {
  float angle = 0.f;
  for (auto& player : expected_players_) {
    auto e_id = connected_player_entities_.find_l(player);
    if (e_id == connected_player_entities_.end()) {
      Logger::err(kLogLabel)
          << "create_initial_game_state could not find player " << player.Id;
      continue;
    }

    entt::entity e = *e_id;
    world_.emplace<component::NetSyncId>(e, next_net_sync_id_++);
    world_.emplace<component::BasicPlayerComponent>(e);
    locomotion_system_.attach_basic_locomotion_components(
        world_, e, glm::vec2(glm::sin(angle) * 5.f, glm::cos(angle) * 5.f),
        3.2f);
    angle += 30.f;

    // TODO (sessamekesh): spawn other entity types
  }
}
