#include <igcore/log.h>
#include <pve_game_scene/ecs/netsync_components.h>
#include <pve_game_scene/ecs/utils.h>

using namespace sanctify;
using namespace ecs;
using namespace indigo;
using namespace core;

namespace {
const float kMaxSnapshotCacheSize = 15;
}

float& ecs::sim_clock_time(entt::registry& world, entt::entity e) {
  return world.get_or_emplace<SimClockComponent>(e, 0.f).simClock;
}

void ecs::queue_client_message(entt::registry& world, entt::entity e,
                               pb::GameClientSingleMessage msg) {
  world.get_or_emplace<OutgoingClientMessagesComponent>(e).messages.push_back(
      msg);
}

void ecs::cache_snapshot_diff(entt::registry& world, entt::entity e,
                              const pb::GameSnapshotDiff& diff_pb) {
  auto& snapshot_cache_component =
      world.get_or_emplace<SnapshotCacheComponent>(e, ::kMaxSnapshotCacheSize);

  auto& snapshot_cache = snapshot_cache_component.cache;
  auto& reconcile_net_state_system =
      snapshot_cache_component.reconcileNetStateSystem;

  float server_clock = sim_clock_time(world, e);

  GameSnapshotDiff diff = GameSnapshotDiff::Deserialize(diff_pb);
  Maybe<GameSnapshot> maybe_snapshot =
      snapshot_cache.assemble_diff_and_store_snapshot(diff);
  if (maybe_snapshot.is_empty()) {
    // Ignore this snapshot, since it cannot be assembled with the current
    // state.
    return;
  }

  GameSnapshot snapshot = maybe_snapshot.move();
  reconcile_net_state_system.register_server_snapshot(snapshot, server_clock);

  pb::GameClientSingleMessage msg{};
  msg.mutable_snapshot_received()->set_snapshot_id(snapshot.snapshot_id());
  queue_client_message(world, e, msg);
}

void ecs::cache_snapshot_full(entt::registry& world, entt::entity e,
                              const pb::GameSnapshotFull& diff_pb) {
  auto& snapshot_cache_component =
      world.get_or_emplace<SnapshotCacheComponent>(e, ::kMaxSnapshotCacheSize);

  auto& snapshot_cache = snapshot_cache_component.cache;
  auto& reconcile_net_state_system =
      snapshot_cache_component.reconcileNetStateSystem;

  float server_clock = sim_clock_time(world, e);

  GameSnapshot snapshot = GameSnapshot::Deserialize(diff_pb);
  snapshot_cache.store_server_snapshot(snapshot);
  reconcile_net_state_system.register_server_snapshot(snapshot, server_clock);

  pb::GameClientSingleMessage msg{};
  msg.mutable_snapshot_received()->set_snapshot_id(snapshot.snapshot_id());
  queue_client_message(world, e, msg);
}

void ecs::add_player_movement_indicator(entt::registry& world,
                                        const pb::PlayerMovement& msg) {
  // TODO (sessamekesh): Add the new player movement indicator!
  const char* kLogLabel = "ecs::add_player_movement_indicator";

  Logger::log(kLogLabel) << "Player movement registered at : "
                         << msg.destination().x() << ", "
                         << msg.destination().y();
}
