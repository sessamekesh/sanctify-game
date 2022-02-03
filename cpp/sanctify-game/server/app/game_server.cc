#include <app/game_server.h>
#include <app/net_components.h>
#include <igcore/log.h>
#include <sanctify-game-common/gameplay/locomotion_components.h>
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

const float kMaxTimeBetweenUpdates = 0.05f;

}  // namespace

GameServer::GameServer()
    : player_message_cb_(::default_player_message_handler),
      is_running_(false),
      sim_clock_(0.f) {}

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

      std::chrono::duration<float> fsec = this_frame - last_frame;
      float dt = fsec.count();

      // Gather inputs
      process_net_events();
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
  auto it = player_entities_.find(player);
  if (it == player_entities_.end()) {
    Logger::log(kLogLabel) << "Skipping message for unregistered player "
                           << player.Id;
    return;
  }

  if (!message.has_game_client_actions_list()) {
    return;
  }

  entt::entity player_entity = it->second;

  for (int i = 0; i < message.game_client_actions_list().actions_size(); i++) {
    const pb::GameClientSingleMessage& action =
        message.game_client_actions_list().actions(i);

    switch (action.msg_body_case()) {
      case pb::GameClientSingleMessage::kTravelToLocationRequest:
        handle_travel_to_location_request(player_entity,
                                          action.travel_to_location_request());
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
void GameServer::handle_travel_to_location_request(
    entt::entity player_entity, const pb::TravelToLocationRequest& request) {
  world_.emplace_or_replace<component::TravelToLocation>(
      player_entity,
      component::TravelToLocation{glm::vec2{request.x(), request.y()}});
}

void GameServer::send_player_updates() {
  // TODO (sessamekesh): Check the player connections, generate diffs to send
  // out, and queue up the updates to be sent to each player. These are partial
  // (many times per second, or when player actions change), or full (every few
  // seconds of simulation time, to catch/deter drift)
  PlayerId players_to_update[64] = {};
  size_t num_players_to_update = 0;

  for (auto it = player_entities_.begin(); it != player_entities_.end(); it++) {
    // TODO (sessamekesh): Gather the entities that need updating
    entt::entity player_entity = it->second;
    component::ClientSyncMetadata& client_meta =
        world_.get<component::ClientSyncMetadata>(player_entity);

    if (sim_clock_ >
        client_meta.LastUpdateSentTime + ::kMaxTimeBetweenUpdates) {
      players_to_update[num_players_to_update++] = client_meta.playerId;
      client_meta.LastUpdateSentTime = sim_clock_;
    }
  }

  // If no players need updating, skip the world serialization step
  if (num_players_to_update == 0u) {
    return;
  }

  // TODO (sessamekesh): Assemble all the components to send back
  // - Set player position
  // - Set player trajectory, speed, and time along trajectory for up to 400ms
  pb::GameServerActionsList server_actions;
  pb::GameServerSingleMessage* player_state_message =
      server_actions.add_messages();

  auto view = world_.view<const component::MapLocation,
                          const component::StandardNavigationParams,
                          const component::ClientSyncMetadata>();
  for (auto [entity, map_location, nav_params, net_meta] : view.each()) {
    pb::PlayerStateUpdate* player_state =
        player_state_message->mutable_player_state_update();

    // TODO (sessamekesh): Use a net sync ID instead of this!
    // You'll want a bimap to pull that off though
    player_state->set_player_id(net_meta.playerId.Id);

    pb::PlayerSingleState* pos_state = player_state->add_player_states();
    pb::MapPosition* pos = pos_state->mutable_current_position();
    pos->set_x(map_location.XZ.x);
    pos->set_y(map_location.XZ.y);

    pb::PlayerSingleState* locomotion_state = player_state->add_player_states();
    pb::PlayerLocomotionState* locomotion =
        locomotion_state->mutable_locomotion_state();

    locomotion->set_speed(nav_params.MovementSpeed);

    auto* dest = world_.try_get<component::TravelToLocation>(entity);
    if (dest != nullptr) {
      pb::TravelToLocationRequest* location =
          locomotion->mutable_target_location();
      component::TravelToLocation::serialize(*dest, location);
    }
  }

  // TODO (sessamekesh): Queue up actions per user that need to be sent, send
  // them in chunks to avoid sending large packets
  pb::GameServerMessage message;
  message.set_magic_number(sanctify::kSanctifyMagicHeader);
  *message.mutable_actions_list() = server_actions;

  for (int i = 0; i < num_players_to_update; i++) {
    player_message_cb_(players_to_update[i], message);
  }
}

void GameServer::update(float dt) {
  sim_clock_ += dt;
  standard_target_travel_system_.update(world_, dt);
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

  auto existing_player = player_entities_.find(evt.playerId);
  if (existing_player != player_entities_.end()) {
    Logger::log(kLogLabel) << "Player " << evt.playerId.Id
                           << " was already connected!";
    // TODO (sessamekesh): reconnect existing player
    return;
  }

  entt::entity new_player_entity = world_.create();
  world_.emplace<component::ClientSyncMetadata>(new_player_entity, evt.playerId,
                                                sim_clock_);
  world_.emplace<component::MapLocation>(new_player_entity,
                                         glm::vec2(0.f, 0.f));
  world_.emplace<component::StandardNavigationParams>(new_player_entity, 1.f);

  player_entities_.insert({evt.playerId, new_player_entity});
  evt.connectionPromise->resolve(true);
}

void GameServer::handle_player_disconnection(DisconnectPlayerEvent& evt) {
  Logger::log(kLogLabel) << "Handling player disconnection - "
                         << evt.playerId.Id;

  auto existing_player = player_entities_.find(evt.playerId);
  if (existing_player == player_entities_.end()) {
    return;
  }

  world_.destroy(existing_player->second);
  player_entities_.erase(existing_player);
}