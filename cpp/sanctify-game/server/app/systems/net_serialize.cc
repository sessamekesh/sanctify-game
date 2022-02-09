#include <app/systems/net_serialize.h>
#include <igcore/log.h>
#include <sanctify-game-common/gameplay/net_sync_components.h>

using namespace sanctify;
using namespace system;
using namespace indigo;
using namespace core;

namespace {
const char* kLogLabel = "NetSerializeSystem";
}

NetSerializeSystem::NetSerializeSystem(float time_between_diffs,
                                       float time_between_snapshots)
    : time_between_diffs_(time_between_diffs),
      time_between_full_snapshots_(time_between_snapshots) {}

void NetSerializeSystem::receive_client_snapshot_ack(const PlayerId& player_id,
                                                     uint32_t snapshot_id) {
  auto& player_it = sent_snapshots_.find(player_id);
  if (player_it == sent_snapshots_.end()) {
    Logger::err(kLogLabel)
        << "Attempting to ack snapshot #" << snapshot_id << " for player "
        << player_id.Id
        << " - but that player has not been sent any snapshots or diffs!";
    return;
  }

  std::map<uint32_t, GameSnapshot>& sent_snapshots = player_it->second;

  // Clear up space (maybe clearing up memory, but that might be optimistic)
  for (auto it = sent_snapshots.begin(); it != sent_snapshots.end();) {
    if (it->first < snapshot_id) {
      it = sent_snapshots.erase(it);

      if (it == sent_snapshots.end()) {
        break;
      }
    } else {
      // TODO (sessamekesh): This assumes a sorted std::map (which it is)
      break;
    }
  }

  ready_players_.insert(player_id);
}

Maybe<GameSnapshot> NetSerializeSystem::get_snapshot_to_send(
    const PlayerId& player_id, entt::registry& world,
    entt::entity player_entity, float server_time) {
  if (ready_players_.count(player_id) > 0 &&
      next_snapshot_time(player_id) > server_time) {
    return empty_maybe{};
  }

  GameSnapshot snapshot =
      gen_snapshot_for_player(server_time, player_id, player_entity, world);
  set_snapshot(player_id, snapshot);

  set_next_snapshot_time(player_id, server_time + time_between_full_snapshots_);
  set_next_diff_time(player_id, server_time + time_between_diffs_);

  return snapshot;
}

Maybe<GameSnapshotDiff> NetSerializeSystem::get_diff_to_send(
    const PlayerId& player_id, entt::registry& world,
    entt::entity player_entity, float server_time) {
  if (next_diff_time(player_id) > server_time) {
    return empty_maybe{};
  }

  Maybe<GameSnapshot> maybe_base_snapshot =
      get_base_snapshot_for_player(player_id);
  if (maybe_base_snapshot.is_empty()) {
    Logger::err(kLogLabel)
        << "Trying to send a diff for a player that has not yet received a "
           "full snapshot - failing, and setting immediate need to send a full "
           "snapshot (player "
        << player_id.Id;
    set_next_snapshot_time(player_id, -1.f);
    return empty_maybe{};
  }

  GameSnapshot base_snapshot = maybe_base_snapshot.move();

  GameSnapshot current_snapshot =
      gen_snapshot_for_player(server_time, player_id, player_entity, world);
  set_snapshot(player_id, current_snapshot);

  return GameSnapshot::CreateDiff(base_snapshot, current_snapshot);
}

float NetSerializeSystem::next_snapshot_time(const PlayerId& player_id) const {
  auto it = time_of_next_full_snapshot_.find(player_id);
  if (it == time_of_next_full_snapshot_.end()) {
    return -1.f;
  }

  return it->second;
}

void NetSerializeSystem::set_next_snapshot_time(const PlayerId& player_id,
                                                float next_time) {
  time_of_next_full_snapshot_[player_id] = next_time;
}

float NetSerializeSystem::next_diff_time(const PlayerId& player_id) const {
  auto it = time_of_next_diff_.find(player_id);
  if (it == time_of_next_diff_.end()) {
    return -1.f;
  }

  return it->second;
}

void NetSerializeSystem::set_next_diff_time(const PlayerId& player_id,
                                            float time) {
  time_of_next_diff_[player_id] = time;
}

void NetSerializeSystem::set_snapshot(const PlayerId& player_id,
                                      GameSnapshot snapshot) {
  uint32_t snapshot_id = snapshot.snapshot_id();

  if (sent_snapshots_.find(player_id) == sent_snapshots_.end()) {
    sent_snapshots_[player_id] = std::map<uint32_t, GameSnapshot>();
  }

  sent_snapshots_[player_id][snapshot_id] = snapshot;
}

uint32_t NetSerializeSystem::get_and_inc_snapshot_id(
    const PlayerId& player_id) {
  auto it = next_snapshot_id_.find(player_id);
  if (it == next_snapshot_id_.end()) {
    next_snapshot_id_[player_id] = 2u;
    return 1u;
  }

  return next_snapshot_id_[player_id]++;
}

GameSnapshot NetSerializeSystem::gen_snapshot_for_player(
    float server_time, const PlayerId& player_id, entt::entity player_entity,
    entt::registry& world) {
  // TODO (sessamekesh): Apply fog of war when you have that working

  GameSnapshot snapshot{};
  snapshot.snapshot_time(server_time);
  snapshot.snapshot_id(get_and_inc_snapshot_id(player_id));

  auto view = world.view<const component::NetSyncId>();

  for (auto [entity, net_sync] : view.each()) {
    component::MapLocation* map_location =
        world.try_get<component::MapLocation>(entity);
    component::NavWaypointList* nav_waypoint =
        world.try_get<component::NavWaypointList>(entity);
    component::StandardNavigationParams* nav_params =
        world.try_get<component::StandardNavigationParams>(entity);

    snapshot.add(net_sync.Id, maybe_from_nullable_ptr(map_location));
    snapshot.add(net_sync.Id, maybe_from_nullable_ptr(nav_waypoint));
    snapshot.add(net_sync.Id, maybe_from_nullable_ptr(nav_params));
  }

  return snapshot;
}

Maybe<GameSnapshot> NetSerializeSystem::get_base_snapshot_for_player(
    const PlayerId& player_id) {
  auto it = sent_snapshots_.find(player_id);
  if (it == sent_snapshots_.end()) {
    return empty_maybe{};
  }

  if (it->second.size() == 0u) {
    return empty_maybe{};
  }

  return it->second.begin()->second;
}
