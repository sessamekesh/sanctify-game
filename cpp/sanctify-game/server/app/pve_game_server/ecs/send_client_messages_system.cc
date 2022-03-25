#include <app/pve_game_server/ecs/net_state_update_system.h>
#include <app/pve_game_server/ecs/netstate_components.h>
#include <app/pve_game_server/ecs/player_context_components.h>
#include <app/pve_game_server/ecs/send_client_messages_system.h>
#include <sanctify-game-common/gameplay/net_sync_components.h>
#include <sanctify-game-common/net/game_snapshot.h>

#include <map>

using namespace sanctify;
using namespace ecs;
using namespace indigo;
using namespace core;

namespace {
const float kTimeBetweenSnapshots = 5.f;
const float kTimeBetweenDiffs = 1.f / 60.f;
const float kUnhealthyTime = 0.1f;

struct SentClientSnapshotsComponent {
  SentClientSnapshotsComponent() : nextSnapshotId(0u) {}

  std::map<uint32_t, GameSnapshot> sentSnapshots;
  uint32_t nextSnapshotId;
};

GameSnapshot gen_snapshot(entt::registry& world, entt::entity e, float sim_time,
                          uint32_t snapshot_id) {
  // TODO (sessamekesh): apply fog of war
  GameSnapshot snapshot{};
  snapshot.snapshot_time(sim_time);
  snapshot.snapshot_id(snapshot_id);

  auto view = world.view<const component::NetSyncId>();
  for (auto [entity, net_sync] : view.each()) {
    component::MapLocation* map_location =
        world.try_get<component::MapLocation>(entity);
    component::NavWaypointList* nav_waypoint =
        world.try_get<component::NavWaypointList>(entity);
    component::StandardNavigationParams* nav_params =
        world.try_get<component::StandardNavigationParams>(entity);
    bool has_basic_player =
        world.all_of<component::BasicPlayerComponent>(entity);
    component::OrientationComponent* orientation =
        world.try_get<component::OrientationComponent>(entity);

    snapshot.add(net_sync.Id, maybe_from_nullable_ptr(map_location));
    snapshot.add(net_sync.Id, maybe_from_nullable_ptr(nav_waypoint));
    snapshot.add(net_sync.Id, maybe_from_nullable_ptr(nav_params));
    snapshot.add(net_sync.Id, maybe_from_nullable_ptr(orientation));

    if (has_basic_player) {
      snapshot.add(net_sync.Id, component::BasicPlayerComponent{});
    }
  }

  return snapshot;
}

void send_full_snapshot(entt::registry& world, entt::entity e, float sim_time,
                        const PlayerId& pid) {
  auto& sc = world.get<SentClientSnapshotsComponent>(e);
  uint32_t snapshot_id = sc.nextSnapshotId++;

  auto snapshot = ::gen_snapshot(world, e, sim_time, snapshot_id);
  auto pb_snapshot = snapshot.serialize();

  pb::GameServerSingleMessage msg{};
  *msg.mutable_game_snapshot_full() = snapshot.serialize();

  sc.sentSnapshots.emplace(snapshot_id, std::move(snapshot));

  ecs::net::queue_single_message(world, e, std::move(msg));
}

void cleanup_stored_snapshots(entt::registry& world, entt::entity e,
                              uint32_t most_recent_recv) {
  auto& sent_snapshots_cache =
      world.get_or_emplace<SentClientSnapshotsComponent>(e);

  for (auto it = sent_snapshots_cache.sentSnapshots.begin();
       it != sent_snapshots_cache.sentSnapshots.end();) {
    if (it->first < most_recent_recv) {
      it = sent_snapshots_cache.sentSnapshots.erase(it);

      if (it == sent_snapshots_cache.sentSnapshots.end()) {
        break;
      }
    } else {
      // This assumes a sorted std::map (which this is)
      break;
    }
  }
}

Maybe<GameSnapshot> get_base_snapshot(entt::registry& world, entt::entity e) {
  auto& sent_snapshots_cache =
      world.get_or_emplace<SentClientSnapshotsComponent>(e);

  auto& snapshot_map = sent_snapshots_cache.sentSnapshots;
  if (snapshot_map.size() == 0u) {
    return empty_maybe{};
  }

  return snapshot_map.begin()->second;
}

void send_diff(entt::registry& world, entt::entity e, const PlayerId& pid,
               const GameSnapshot& base, float sim_time) {
  auto& sc = world.get<SentClientSnapshotsComponent>(e);
  uint32_t snapshot_id = sc.nextSnapshotId++;
  GameSnapshot current_snapshot =
      ::gen_snapshot(world, e, sim_time, snapshot_id);

  auto diff = GameSnapshot::CreateDiff(base, current_snapshot);

  sc.sentSnapshots.emplace(snapshot_id, std::move(current_snapshot));

  pb::GameServerSingleMessage msg{};
  *msg.mutable_game_snapshot_diff() = diff.serialize();

  ecs::net::queue_single_message(world, e, std::move(msg));
}

}  // namespace

void QueueClientMessagesSystem::update(entt::registry& world, float sim_time) {
  auto view =
      world.view<const ecs::PlayerConnectionState,
                 const ecs::PlayerSystemAttributes, PlayerNetStateComponent>();

  for (auto [e, player_connect_state, player_attribs, net_state] :
       view.each()) {
    // Early out - do not queue any messages for non-receptive client
    if (player_connect_state.netState !=
            ecs::PlayerConnectionState::NetState::Healthy ||
        !player_connect_state.isReady) {
      world.remove<SentClientSnapshotsComponent>(e);
      continue;
    }

    const auto& player_id = player_attribs.playerId;

    ::cleanup_stored_snapshots(world, e, net_state.lastAckedSnapshotId);

    if (net_state.lastAckedSnapshotId == 0u ||
        (net_state.lastSnapshotSentTime + ::kTimeBetweenSnapshots) < sim_time) {
      ::send_full_snapshot(world, e, sim_time, player_id);
      net_state.lastSnapshotSentTime = sim_time;
      continue;
    }

    if ((net_state.lastDiffSentTime + ::kTimeBetweenDiffs) < sim_time) {
      auto maybe_base_snapshot = ::get_base_snapshot(world, e);
      if (maybe_base_snapshot.is_empty()) {
        ::send_full_snapshot(world, e, sim_time, player_id);
        net_state.lastSnapshotSentTime = sim_time;
        net_state.lastDiffSentTime = sim_time;
        continue;
      }

      ::send_diff(world, e, player_id, maybe_base_snapshot.get(), sim_time);
      net_state.lastDiffSentTime = sim_time;
    }
  }
}
