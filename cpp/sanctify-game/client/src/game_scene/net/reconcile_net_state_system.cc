#include <game_scene/net/reconcile_net_state_system.h>
#include <igcore/maybe.h>

using namespace sanctify;
using namespace indigo;
using namespace core;

namespace {
const char* kLogLabel = "ReconcileNetStateSystem";

template <typename T>
void maybe_upsert(entt::entity entity, entt::registry& world,
                  Maybe<T> component) {
  if (component.has_value()) {
    auto& c = world.emplace_or_replace<T>(entity);
    c = component.move();
  }
}

template <typename T>
void maybe_tag(entt::entity entity, entt::registry& world, Maybe<T> tag) {
  if (tag.has_value()) {
    world.emplace_or_replace<T>(entity);
  }
}

}  // namespace

ReconcileNetStateSystem::ReconcileNetStateSystem() {}

void ReconcileNetStateSystem::advance_time_to_and_maybe_reconcile(
    entt::registry& client_world,
    std::function<void(entt::registry& server_sim, float dt)>
        update_client_sim_cb,
    float client_sim_time) {
  auto last_matching_it = snapshots_.end();

  for (auto it = snapshots_.begin(); it != snapshots_.end();) {
    if (it->snapshot.snapshot_time() <= client_sim_time) {
      last_matching_it = it++;
      if (it != snapshots_.end() &&
          it->snapshot.snapshot_time() <= client_sim_time) {
        snapshots_.erase(last_matching_it);
        last_matching_it = snapshots_.end();
      }
    } else {
      break;
    }
  }

  if (last_matching_it == snapshots_.end()) {
    // No snapshots found before the future point
    return;
  }

  reconcile_client_state(client_world, client_sim_time, update_client_sim_cb,
                         last_matching_it->snapshot);
}

void ReconcileNetStateSystem::register_server_snapshot(
    const GameSnapshot& server_snapshot, float client_sim_time) {
  // If this snapshot is in the past and there is a more recent snapshot,
  // discard this one
  for (auto it = snapshots_.begin(); it != snapshots_.end(); it++) {
    if (it->snapshot.snapshot_time() <= client_sim_time &&
        it->snapshot.snapshot_time() > server_snapshot.snapshot_time()) {
      // Snapshot is in the past, but we have a more recent snapshot - discard
      // this one
      return;
    }

    if (it->snapshot.snapshot_time() > client_sim_time) {
      // Future snapshots should be rare, and also not considered here. This
      // snapshot should be added.
      break;
    }
  }

  // TODO (sessamekesh): This sorting takes too long!
  snapshots_.insert({server_snapshot});
}

void ReconcileNetStateSystem::reconcile_client_state(
    entt::registry& client_world, float client_sim_time,
    std::function<void(entt::registry& server_sim, float dt)>
        update_client_sim_cb,
    const GameSnapshot& server_snapshot) {
  snapshot_translator_.write_fresh_game_state(server_state_, server_snapshot);

  update_client_sim_cb(server_state_,
                       client_sim_time - server_snapshot.snapshot_time());

  sanctify::GameSnapshot current_server_snapshot =
      snapshot_translator_
          .read_all_game_state(server_state_, 0, client_sim_time)
          .gameSnapshot;
  auto current_client_state = snapshot_translator_.read_all_game_state(
      client_world, 0, client_sim_time);

  GameSnapshotDiff client_diff = GameSnapshot::CreateDiff(
      current_client_state.gameSnapshot, current_server_snapshot);

  Bimap<uint32_t, entt::entity>& entityBimap = current_client_state.entityBimap;

  //
  // Apply the diff
  //

  // Step 1: Delete all entities that are no longer in use
  PodVector<uint32_t> deleted_entities = client_diff.deleted_entities();
  for (int i = 0; i < deleted_entities.size(); i++) {
    uint32_t deleted_net_client_id = deleted_entities[i];

    auto deleted_entity_it = entityBimap.find_l(deleted_net_client_id);
    if (deleted_entity_it == entityBimap.end()) {
      continue;
    }

    entt::entity e = *deleted_entity_it;

    client_world.destroy(e);
  }

  // Step 2: remove any components that are removed on the server side
  // Step 3: wholesale update any components that can be drop-replaced
  // Step 4: apply smearing to components that need smearing
  PodVector<uint32_t> modified_entities = client_diff.upserted_entities();
  for (int i = 0; i < modified_entities.size(); i++) {
    uint32_t net_client_id = modified_entities[i];

    entt::entity e = entt::null;
    auto modified_entity_it = entityBimap.find_l(net_client_id);
    if (modified_entity_it == entityBimap.end()) {
      e = client_world.create();
      client_world.emplace<component::NetSyncId>(e, net_client_id);
      entityBimap.insert(net_client_id, e);
    } else {
      e = *modified_entity_it;
    }

    // Step 2: remove any components that are removed on the server side
    PodVector<GameSnapshotDiff::ComponentType> deleted_components =
        client_diff.deleted_components(net_client_id);
    for (int j = 0; j < deleted_components.size(); j++) {
      GameSnapshotDiff::ComponentType deleted_component = deleted_components[j];

      switch (deleted_component) {
        case GameSnapshotDiff::ComponentType::MapLocation:
          client_world.erase<component::MapLocation>(e);
          break;
        case GameSnapshotDiff::ComponentType::NavWaypointList:
          client_world.erase<component::NavWaypointList>(e);
          break;
        case GameSnapshotDiff::ComponentType::StandardNavigationParams:
          client_world.erase<component::StandardNavigationParams>(e);
          break;
        case GameSnapshotDiff::ComponentType::BasicPlayerComponent:
          client_world.erase<component::BasicPlayerComponent>(e);
          break;
        case GameSnapshotDiff::ComponentType::Orientation:
          client_world.erase<component::OrientationComponent>(e);
          break;
        default:
          Logger::err(kLogLabel)
              << "Unknown ComponentType: " << (uint32_t)deleted_component;
      }
    }

    // Step 3: wholesale update any components that can be drop-replaced
    ::maybe_upsert(e, client_world,
                   client_diff.nav_waypoint_list(net_client_id));
    ::maybe_upsert(e, client_world,
                   client_diff.standard_navigation_params(net_client_id));
    ::maybe_tag(e, client_world,
                client_diff.basic_player_component(net_client_id));

    // Step 4: apply smearing to components that need smearing
    // TODO (sessamekesh): apply smearing to map position
    ::maybe_upsert(e, client_world, client_diff.map_location(net_client_id));
    ::maybe_upsert(e, client_world, client_diff.orientation(net_client_id));
  }
}

bool ReconcileNetStateSystem::TimeSnapshot::operator<(
    const ReconcileNetStateSystem::TimeSnapshot& o) const {
  return snapshot.snapshot_time() < o.snapshot.snapshot_time();
}

bool ReconcileNetStateSystem::TimeSnapshot::operator==(
    const ReconcileNetStateSystem::TimeSnapshot& o) const {
  return snapshot.snapshot_time() == o.snapshot.snapshot_time();
}
