#include <app/pve_game_server/ecs/context_components.h>
#include <app/pve_game_server/ecs/netstate_components.h>
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
  pb::GameServerPlayerDescription player_desc{};
  player_desc.set_display_name("sessamekesh");
  player_desc.set_tagline("I am a test player");
  player_desc.set_player_id((uint64_t)std::hash<std::string>{}("test-token"));
  player_desc.set_player_token("test-token");
  ecs::add_expected_player(world_, player_desc);
  auto& expected_players = world_.ctx<ecs::GExpectedPlayers>().entityMap;

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

        Logger::log(kLogLabel) << "Waiting for players...";
        server_stage_ = ServerStage::WaitingForPlayers;

        // Bootstrap the game server world state...
        ecs::bootstrap_game_engine_callbacks(world_, player_message_cb_);
        ecs::bootstrap_server_config(world_, /** max_actions_per_message= */ 32,
                                     /** max_queued_client_messages= */ 16,
                                     /** max_queued_server_actions= */ 256);
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

// TODO (sessamekesh): This should use direct tokens since player tokens AND
//  player IDs are known by the server at startup!
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

  if (ecs::get_player_entity(world_, pid).is_empty()) {
    Logger::log(kLogLabel)
        << "Attempted to have a world join by player with bad ID " << pid.Id;
    return Promise<bool>::immediate(false);
  }

  if (!net_event_organizer_.add_player(pid)) {
    return Promise<bool>::immediate(false);
  }

  return Promise<bool>::immediate(true);
}

std::shared_ptr<Promise<EmptyPromiseRsl>> PveGameServer::shutdown() {
  return shutdown_promise_;
}

void PveGameServer::update(float dt) {
  ecs::tick(world_, dt);

  switch (server_stage_) {
    case ServerStage::Initializing:
    case ServerStage::Terminated:
      return;

    case ServerStage::WaitingForPlayers:
      update_waiting_for_players();
      break;

    case ServerStage::Running:
      update_running();
      break;

    case ServerStage::GameOver:
      update_game_over();
      break;
  }

  auto& players = world_.ctx<ecs::GExpectedPlayers>();
  for (const auto& entity_pair : players.entityMap) {
    ecs::net::flush_messages(world_, entity_pair.second);
  }
}

void PveGameServer::update_waiting_for_players() {
  const auto& entity_map = world_.ctx<ecs::GExpectedPlayers>().entityMap;
  for (const auto& player_pair : entity_map) {
    PlayerId pid = player_pair.first;
    entt::entity e = player_pair.second;
    if (!world_.valid(e)) {
      continue;
    }

    maybe_queue_pong(pid, e);
  }

  // Run netsync code...
  net_state_update_system_.update(world_, net_event_organizer_);
  queue_client_messages_system_.update(world_, ecs::sim_time(world_));

  bool all_ready = true;
  for (const auto& player_pair :
       world_.ctx<ecs::GExpectedPlayers>().entityMap) {
    all_ready =
        all_ready && net_event_organizer_.ready_state(player_pair.first);
  }

  if (all_ready) {
    Logger::log(kLogLabel) << "All players ready - starting the simulation!";
    create_initial_game_scene();
    server_stage_ = ServerStage::Running;
    return;
  }
}

void PveGameServer::update_running() {
  // Handle player input events...
  for (const auto& player_pair :
       world_.ctx<ecs::GExpectedPlayers>().entityMap) {
    PlayerId pid = player_pair.first;
    entt::entity e = player_pair.second;
    if (!world_.valid(e)) {
      continue;
    }

    maybe_queue_pong(pid, e);

    auto movement_evt = net_event_organizer_.get_move_command(pid);
    if (movement_evt.has_value()) {
      Logger::log(kLogLabel) << "Attaching move command for player " << pid.Id;
      component::PlayerNavRequestComponent::attach_on(world_, e,
                                                      movement_evt.get());
    }

    auto last_snapshot_recvd = net_event_organizer_.last_acked_snapshot_id(pid);
    if (last_snapshot_recvd.has_value()) {
      auto* net_state = world_.try_get<ecs::PlayerNetStateComponent>(e);
      if (net_state != nullptr &&
          net_state->lastAckedSnapshotId < last_snapshot_recvd.get()) {
        net_state->lastAckedSnapshotId = last_snapshot_recvd.get();
      }
    }
  }

  // Handle server logic (including sending out messages to clients!)
  net_state_update_system_.update(world_, net_event_organizer_);
  player_nav_system_.update(world_, navmesh_.get());
  locomotion_system_.apply_standard_locomotion(world_, ecs::frame_time(world_));
  queue_client_messages_system_.update(world_, ecs::sim_time(world_));
}

void PveGameServer::update_game_over() {
  // ha
}

void PveGameServer::maybe_queue_pong(const PlayerId& pid, entt::entity e) {
  auto ping_record_opt = net_event_organizer_.ping_record(pid);
  if (ping_record_opt.is_empty()) {
    return;
  }
  auto ping_record = ping_record_opt.move();

  // TODO (sessamekesh): telemetry against the ping

  pb::GameServerSingleMessage msg;
  auto* pong_msg = msg.mutable_pong();
  pong_msg->set_sim_time(ecs::sim_time(world_));
  if (ping_record.pingId.has_value()) {
    pong_msg->set_ping_id(ping_record.pingId.get());
  }
  ecs::net::queue_single_message(world_, e, std::move(msg));
}

void PveGameServer::create_initial_game_scene() {
  // TODO (sessamekesh): Move this to a utility function outside of the game
  //  server class
  float angle = 0.f;
  for (const auto& player_pair :
       world_.ctx<ecs::GExpectedPlayers>().entityMap) {
    entt::entity e = player_pair.second;
    world_.emplace<component::NetSyncId>(e, next_net_sync_id_++);
    world_.emplace<component::BasicPlayerComponent>(e);
    locomotion_system_.attach_basic_locomotion_components(
        world_, e, glm::vec2(glm::sin(angle) * 5.f, glm::cos(angle) * 5.f),
        3.2f);
    angle += 30.f;

    // TODO (sessamekesh): spawn other entity types
  }
}
