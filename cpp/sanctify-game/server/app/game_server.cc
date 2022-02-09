#include <app/game_server.h>
#include <app/net_components.h>
#include <igcore/log.h>
#include <sanctify-game-common/gameplay/locomotion_components.h>
#include <sanctify-game-common/gameplay/net_sync_components.h>
#include <sanctify-game-common/net/net_config.h>
#include <util/visit.h>

#include <chrono>

using namespace indigo;
using namespace core;
using namespace sanctify;

using hrclock = std::chrono::high_resolution_clock;
using namespace std::chrono_literals;

namespace {
const char* kLogLabel = "GameServer";

bool default_player_message_handler(PlayerId player_id,
                                    sanctify::pb::GameServerMessage msg) {
  Logger::err(kLogLabel)
      << "No player message handler defined - message for player "
      << player_id.Id << " is dropped!";

  return false;
}

bool is_receptive_net_state(NetServer::PlayerConnectionState state) {
  switch (state) {
    case NetServer::PlayerConnectionState::ConnectedBasic:
    case NetServer::PlayerConnectionState::ConnectedPreferred:
      return true;

    default:
      return false;
  }
}

bool is_reconnectable_state(NetServer::PlayerConnectionState state) {
  switch (state) {
    case NetServer::PlayerConnectionState::Disconnected:
    case NetServer::PlayerConnectionState::Unhealthy:
      return true;
    default:
      return false;
  }
}

const float kMaxTimeBetweenFullSyncs = 10.f;
const float kMaxTimeBetweenUpdates = 0.1f;

}  // namespace

GameServer::GameServer()
    : player_message_cb_(::default_player_message_handler),
      is_running_(false),
      sim_clock_(0.f),
      next_net_sync_id_(1u),
      net_serialize_system_(kMaxTimeBetweenUpdates, kMaxTimeBetweenFullSyncs) {}

GameServer::~GameServer() {}

std::shared_ptr<GameServer::GameInitializePromise> GameServer::start_game() {
  auto rsl = GameInitializePromise::create();
  game_thread_ = std::thread([this, lifetime = shared_from_this(), rsl]() {
    Logger::log(kLogLabel) << "Beginning GameServer application loop on thread "
                           << std::this_thread::get_id();

    is_running_ = true;

    auto last_frame = hrclock::now();
    std::this_thread::sleep_for(2ms);
    while (is_running_) {
      auto this_frame = hrclock::now();

      using FpSeconds =
          std::chrono::duration<float, std::chrono::seconds::period>;
      float dt = FpSeconds(this_frame - last_frame).count();

      last_frame = this_frame;

      // Gather inputs
      process_net_events();
      apply_player_inputs();
      update(dt);
      send_player_updates();

      // Wait for next tick
      // TODO (sessamekesh): Do this with a proper frame synchronizer process
      std::this_thread::sleep_for(10ms);
    }
  });

  return rsl;
}

void GameServer::receive_message_for_player(
    PlayerId player_id, sanctify::pb::GameClientMessage client_msg) {
  message_queue_.enqueue({player_id, std::move(client_msg)});
}

void GameServer::set_player_message_receiver(PlayerMessageCallback cb) {
  player_message_cb_ = cb;
}

std::shared_ptr<Promise<bool>> GameServer::try_connect_player(
    const PlayerId& player_id) {
  Logger::log(kLogLabel) << "Received request to connect player "
                         << player_id.Id << " - enqueuing for next frame...";
  auto p = Promise<bool>::create();
  NetEvent next_event = ConnectPlayerEvent{player_id, p};
  net_events_queue_.enqueue(std::move(next_event));
  return p;
}

void GameServer::notify_player_dropped_connection(const PlayerId& player_id) {
  net_events_queue_.enqueue(DisconnectPlayerEvent{player_id});
}

std::shared_ptr<indigo::core::Promise<indigo::core::EmptyPromiseRsl>>
GameServer::shutdown() {
  // Begin shutdown process, and resolve the promise when the game server has
  // completed its shutdown
  is_running_ = false;
  return Promise<EmptyPromiseRsl>::immediate(EmptyPromiseRsl{});
}

//
// Game Logic
//
void GameServer::apply_player_inputs() {
  // Dequeue messages in groups of 5 just as a slight efficiency optimization
  std::pair<PlayerId, pb::GameClientMessage> messages[5];
  std::size_t count = 0;
  std::size_t total_count = 0;
  while ((count = message_queue_.try_dequeue_bulk(messages, 5)) != 0) {
    total_count += count;

    for (int i = 0; i < count; i++) {
      apply_single_player_input(messages[i].first, messages[i].second);
    }

    // Don't waste too much time here - make forward progress, process a bunch
    // of messages, and then carry on.
    if (total_count > 50) {
      return;
    }
  }
}

void GameServer::apply_single_player_input(
    const PlayerId& player, const pb::GameClientMessage& message) {
  entt::entity player_entity{};

  {
    std::shared_lock<std::shared_mutex> l(mut_connected_players_);
    auto it = connected_players_.find(player);
    if (it == connected_players_.end()) {
      Logger::log(kLogLabel)
          << "Skipping message for unregistered player " << player.Id;
      return;
    }
    player_entity = it->second.playerEntity;
  }

  if (!message.has_game_client_actions_list()) {
    return;
  }

  for (int i = 0; i < message.game_client_actions_list().actions_size(); i++) {
    const pb::GameClientSingleMessage& action =
        message.game_client_actions_list().actions(i);

    switch (action.msg_body_case()) {
      case pb::GameClientSingleMessage::MsgBodyCase::kTravelToLocationRequest:
        handle_travel_to_location(player_entity,
                                  action.travel_to_location_request());
        break;
      case pb::GameClientSingleMessage::MsgBodyCase::kSnapshotReceived:
        net_serialize_system_.receive_client_snapshot_ack(
            player, action.snapshot_received().snapshot_id());
        break;
      default:
        Logger::log(kLogLabel)
            << "Unrecognized action type - " << (int)action.msg_body_case()
            << " - when processing messages for player " << player.Id;
        break;
    }
  }
}

//
// Net client handlers
//

void GameServer::send_player_updates() {
  Vector<std::pair<PlayerId, entt::entity>> receptive_players(10);

  {
    std::shared_lock<std::shared_mutex> l(mut_connected_players_);
    for (const auto& it : connected_players_) {
      if (!::is_receptive_net_state(it.second.netState)) {
        continue;
      }

      receptive_players.push_back({it.first, it.second.playerEntity});
    }
  }

  float server_time = sim_clock_;

  for (int i = 0; i < receptive_players.size(); i++) {
    const auto& player_pair = receptive_players[i];
    Maybe<GameSnapshot> full_snapshot =
        net_serialize_system_.get_snapshot_to_send(
            player_pair.first, world_, player_pair.second, sim_clock_);

    // TODO (sessamekesh): Instead of sending an individual message, queue up
    //  actions that are sent at the bottom of a frame!
    if (full_snapshot.has_value()) {
      pb::GameServerMessage msg{};
      *msg.mutable_actions_list()
           ->add_messages()
           ->mutable_game_snapshot_full() = full_snapshot.get().serialize();
      player_message_cb_(player_pair.first, msg);
      continue;
    }

    Maybe<GameSnapshotDiff> diff = net_serialize_system_.get_diff_to_send(
        player_pair.first, world_, player_pair.second, sim_clock_);
    if (diff.has_value()) {
      pb::GameServerMessage msg{};
      *msg.mutable_actions_list()
           ->add_messages()
           ->mutable_game_snapshot_diff() = diff.get().serialize();
      player_message_cb_(player_pair.first, msg);
      continue;
    }
  }
}

void GameServer::handle_travel_to_location(
    entt::entity player_entity,
    const pb::PlayerMovement& travel_to_location_request) {
  server_locomotion_system_.handle_player_movement_event(
      travel_to_location_request, player_entity, world_);
}

void GameServer::update(float dt) {
  sim_clock_ += dt;
  locomotion_system_.apply_standard_locomotion(world_, dt);
}

void GameServer::process_net_events() {
  // Go through all net events that have been set, and handle them (up to 64)
  constexpr size_t kMaxEvents = 12;
  NetEvent unhandled_events[kMaxEvents];

  size_t num_events =
      net_events_queue_.try_dequeue_bulk(unhandled_events, kMaxEvents);

  for (int i = 0; i < num_events; i++) {
    handle_net_event(unhandled_events[i]);
  }
}

void GameServer::handle_net_event(NetEvent& evt) {
  Visit::visit_if_t<ConnectPlayerEvent>(evt, [this](ConnectPlayerEvent& evt) {
    handle_connect_player_event(evt);
  });
  Visit::visit_if_t<DisconnectPlayerEvent>(
      evt,
      [this](DisconnectPlayerEvent& evt) { handle_player_disconnection(evt); });
}

void GameServer::handle_connect_player_event(ConnectPlayerEvent& evt) {
  Logger::log(kLogLabel) << "Handling player connection for player: "
                         << evt.playerId.Id;

  std::unique_lock<std::shared_mutex> l(mut_connected_players_);
  auto existing_player = connected_players_.find(evt.playerId);
  if (existing_player != connected_players_.end()) {
    if (::is_reconnectable_state(existing_player->second.netState)) {
      Logger::log(kLogLabel)
          << "Player " << evt.playerId.Id
          << " reconnecting (was previously in unhealthy/disconnected state)";
      evt.connectionPromise->resolve(true);
    } else {
      Logger::log(kLogLabel)
          << "Player " << evt.playerId.Id
          << " is already connected and in a non-connectable state";
    }
    return;
  }

  entt::entity player_entity = world_.create();
  locomotion_system_.attach_basic_locomotion_components(
      world_, player_entity,
      /* map_position */ glm::vec2(0.f, 0.f),
      /* movement_speed */ 8.f);
  world_.emplace<component::NetSyncId>(player_entity, next_net_sync_id_++);

  // TODO (sessamekesh): Remove this hack!!!
  PodVector<glm::vec2> waypoints(4);
  waypoints.push_back(glm::vec2{0.f, 80.f});
  waypoints.push_back(glm::vec2{-2.f, -80.f});
  waypoints.push_back(glm::vec2{15.f, 55.f});
  waypoints.push_back(glm::vec2{-5.f, -250.f});
  world_.emplace<component::NavWaypointList>(player_entity, waypoints);
  // END HACK

  ConnectedPlayerState player_state{
      evt.playerId, player_entity,
      NetServer::PlayerConnectionState::ConnectedBasic};
  connected_players_.emplace(evt.playerId, std::move(player_state));

  evt.connectionPromise->resolve(true);
}

void GameServer::handle_player_disconnection(DisconnectPlayerEvent& evt) {
  Logger::log(kLogLabel) << "Handling player disconnection - "
                         << evt.playerId.Id;

  std::unique_lock<std::shared_mutex> l(mut_connected_players_);
  auto existing_player = connected_players_.find(evt.playerId);
  if (existing_player == connected_players_.end()) {
    return;
  }

  world_.destroy(existing_player->second.playerEntity);
  connected_players_.erase(existing_player);
}

void GameServer::set_player_connection_state(
    const PlayerId& player_id,
    NetServer::PlayerConnectionState connection_state) {
  std::unique_lock<std::shared_mutex> l(mut_connected_players_);
  auto it = connected_players_.find(player_id);
  if (it != connected_players_.end()) {
    it->second.netState = connection_state;

    // TODO (sessamekesh): handle state transitions here
  }
}

Maybe<entt::entity> GameServer::get_player_entity(
    const PlayerId& player) const {
  std::shared_lock<std::shared_mutex> l(mut_connected_players_);
  auto it = connected_players_.find(player);
  if (it == connected_players_.end()) {
    return empty_maybe{};
  }

  return it->second.playerEntity;
}