#include <app/pve_game_server/net_event_organizer.h>
#include <igcore/log.h>

using namespace sanctify;
using namespace indigo;
using namespace core;

namespace {
const char* kLogLabel = "NetEventOrganizer (PvE)";
}

NetEventOrganizer::NetEventOrganizer(uint32_t num_players)
    : num_players_(num_players),
      m_player_id_(num_players),
      player_ids_(num_players),
      m_move_commands_(num_players),
      move_commands_(num_players),
      m_last_acked_snapshots_(num_players),
      last_acked_snapshots_(num_players),
      m_netserver_state_(num_players),
      netserver_states_(num_players),
      m_ready_state_(num_players),
      ready_states_(num_players),
      m_ping_record_(num_players),
      ping_records_(num_players) {
  for (int i = 0; i < num_players; i++) {
    ready_states_[i] = false;
  }
}

bool NetEventOrganizer::add_player(const PlayerId& player_id) {
  for (int i = 0; i < num_players_; i++) {
    std::unique_lock<std::shared_mutex> l(m_player_id_[i]);
    if (player_ids_[i] == player_id) {
      // Already connected! Don't need to re-connect.
      return true;
    }
    if (player_ids_[i].is_empty()) {
      player_ids_[i] = player_id;
    }
    return true;
  }

  Logger::err(kLogLabel) << "Attempted to add player " << player_id.Id
                         << " when all " << num_players_
                         << " slots have already been filled!";
  return false;
}

bool NetEventOrganizer::remove_player(const PlayerId& player_id) {
  for (int i = 0; i < num_players_; i++) {
    std::unique_lock<std::shared_mutex> l(m_player_id_[i]);
    if (player_ids_[i] == player_id) {
      player_ids_[i] = empty_maybe{};
      return true;
    }
  }
  return false;
}

void NetEventOrganizer::recv_message(const PlayerId& player_id,
                                     pb::GameClientMessage msg) {
  if (!msg.has_game_client_actions_list()) {
    return;
  }

  const auto& actions = msg.game_client_actions_list();
  for (const auto& action : actions.actions()) {
    if (action.has_travel_to_location_request()) {
      travel_to_location_request_event(player_id,
                                       action.travel_to_location_request());
    } else if (action.has_snapshot_received()) {
      snapshot_received_event(player_id, action.snapshot_received());
    } else if (action.has_client_ping()) {
      handle_ping(player_id, action.client_ping());
    } else {
      Logger::err(kLogLabel) << "Unexpected action type from server: "
                             << (int)action.msg_body_case();
    }
  }
}

Maybe<glm::vec2> NetEventOrganizer::get_move_command(const PlayerId& pid) {
  auto idx = row_idx(pid);
  if (idx.is_empty()) {
    return empty_maybe{};
  }

  std::lock_guard<std::mutex> l(m_move_commands_[idx.get()]);
  return std::move(move_commands_[idx.get()]);
}

Maybe<uint32_t> NetEventOrganizer::last_acked_snapshot_id(const PlayerId& pid) {
  auto idx = row_idx(pid);
  if (idx.is_empty()) {
    return empty_maybe{};
  }

  std::lock_guard<std::mutex> l(m_last_acked_snapshots_[idx.get()]);
  return last_acked_snapshots_[idx.get()];
}

bool NetEventOrganizer::ready_state(const PlayerId& pid) const {
  auto idx = row_idx(pid);
  if (idx.is_empty()) {
    return false;
  }

  std::lock_guard<std::mutex> l(m_ready_state_[idx.get()]);
  return ready_states_[idx.get()];
}

Maybe<NetEventOrganizer::PingRecord> NetEventOrganizer::ping_record(
    const PlayerId& pid) {
  auto idx = row_idx(pid);
  if (idx.is_empty()) {
    return empty_maybe{};
  }

  std::lock_guard<std::mutex> l(m_ping_record_[idx.get()]);
  return std::move(ping_records_[idx.get()]);
}

NetServer::PlayerConnectionState NetEventOrganizer::net_server_state(
    const PlayerId& pid) {
  auto idx = row_idx(pid);
  if (idx.is_empty()) {
    return NetServer::PlayerConnectionState::Unknown;
  }

  std::lock_guard<std::mutex> l(m_netserver_state_[idx.get()]);
  return netserver_states_[idx.get()];
}

void NetEventOrganizer::set_net_server_state(
    const PlayerId& pid, NetServer::PlayerConnectionState state) {
  auto idx = row_idx(pid);
  if (idx.is_empty()) {
    return;
  }

  {
    std::lock_guard<std::mutex> l(m_netserver_state_[idx.get()]);
    netserver_states_[idx.get()] = state;
  }

  if (state == NetServer::PlayerConnectionState::Disconnected) {
    std::lock_guard<std::mutex> l(m_ready_state_[idx.get()]);
    ready_states_[idx.get()] = false;
  }
}

Maybe<uint32_t> NetEventOrganizer::row_idx(const PlayerId& player_id) const {
  for (int i = 0; i < num_players_; i++) {
    std::shared_lock<std::shared_mutex> l(m_player_id_[i]);
    if (player_ids_[i] == player_id) {
      return i;
    }
  }

  return empty_maybe{};
}

void NetEventOrganizer::travel_to_location_request_event(
    const PlayerId& player_id, const pb::PlayerMovement& msg) {
  // TODO (sessamekesh): have a way to discard old events (know what the
  //  client time of sending the event was?)
  auto idx = row_idx(player_id);
  if (idx.is_empty()) {
    return;
  }

  if (!msg.has_destination()) {
    return;
  }

  glm::vec2 dest(msg.destination().x(), msg.destination().y());

  std::lock_guard<std::mutex> l(m_move_commands_[idx.get()]);
  move_commands_[idx.get()] = dest;
}

void NetEventOrganizer::snapshot_received_event(
    const PlayerId& player_id, const pb::SnapshotReceived& msg) {
  auto idx = row_idx(player_id);
  if (idx.is_empty()) {
    return;
  }

  std::lock_guard<std::mutex> l(m_last_acked_snapshots_[idx.get()]);
  if (last_acked_snapshots_[idx.get()].is_empty() ||
      last_acked_snapshots_[idx.get()].get() < msg.snapshot_id()) {
    last_acked_snapshots_[idx.get()] = msg.snapshot_id();
  }
}

void NetEventOrganizer::handle_ping(const PlayerId& player_id,
                                    const pb::ClientPing& ping) {
  auto idx = row_idx(player_id);
  if (idx.is_empty()) {
    return;
  }

  // TODO (sessamekesh): additional health/telemetry logic can be held here too

  {
    std::lock_guard<std::mutex> l(m_ready_state_[idx.get()]);

    if (ping.is_ready() && !ready_states_[idx.get()]) {
      Logger::log(kLogLabel) << "Player " << player_id.Id << " ready!";
    }

    ready_states_[idx.get()] = ping.is_ready();
  }

  Maybe<uint32_t> ping_id;
  if (ping.ping_id() > 0) {
    ping_id = ping.ping_id();
  }

  {
    std::lock_guard<std::mutex> l(m_ping_record_[idx.get()]);
    ping_records_[idx.get()] = PingRecord{ping_id, ping.local_time()};
  }
}
