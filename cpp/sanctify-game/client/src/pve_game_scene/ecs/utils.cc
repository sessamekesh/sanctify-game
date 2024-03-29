#include <igcore/log.h>
#include <pve_game_scene/ecs/movement_indicator_render_system.h>
#include <pve_game_scene/ecs/netsync_components.h>
#include <pve_game_scene/ecs/utils.h>

using namespace sanctify;
using namespace ecs;
using namespace indigo;
using namespace core;

namespace {
const float kMaxSnapshotCacheSize = 15;
uint32_t kMaxMessageQueueLength = 256;
}  // namespace

float& ecs::sim_clock_time(entt::registry& world) {
  return world.ctx_or_set<SimClockComponent>(0.f).simClock;
}

void ecs::queue_client_message(entt::registry& world,
                               pb::GameClientSingleMessage msg) {
  auto& msg_queue =
      world.ctx_or_set<OutgoingClientMessagesComponent>().messages;
  if (msg_queue.size() < kMaxMessageQueueLength) {
    msg_queue.push_back(msg);
  } else {
    Logger::log("ecs::queue_client_message")
        << "Message buffer full! Cannot queue new message";
  }
}

void ecs::flush_message_queue(entt::registry& world,
                              std::shared_ptr<NetClient> net_client,
                              uint32_t max_message_count) {
  auto& messages = world.ctx_or_set<OutgoingClientMessagesComponent>().messages;

  pb::GameClientMessage msg{};
  auto* actions_list = msg.mutable_game_client_actions_list();
  for (int i = 0; i < messages.size() && i < max_message_count; i++) {
    *actions_list->add_actions() = messages[i];
  }
  messages.trim_front(max_message_count);
  net_client->send_message(std::move(msg));
}

void ecs::cache_snapshot_diff(entt::registry& world,
                              const pb::GameSnapshotDiff& diff_pb) {
  auto& snapshot_cache_component =
      world.ctx_or_set<SnapshotCacheComponent>(::kMaxSnapshotCacheSize);

  auto& snapshot_cache = snapshot_cache_component.cache;
  auto& reconcile_net_state_system =
      snapshot_cache_component.reconcileNetStateSystem;

  float server_clock = sim_clock_time(world);

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
  queue_client_message(world, msg);
}

void ecs::cache_snapshot_full(entt::registry& world,
                              const pb::GameSnapshotFull& diff_pb) {
  auto& snapshot_cache_component =
      world.ctx_or_set<SnapshotCacheComponent>(::kMaxSnapshotCacheSize);

  auto& snapshot_cache = snapshot_cache_component.cache;
  auto& reconcile_net_state_system =
      snapshot_cache_component.reconcileNetStateSystem;

  float server_clock = sim_clock_time(world);

  GameSnapshot snapshot = GameSnapshot::Deserialize(diff_pb);
  snapshot_cache.store_server_snapshot(snapshot);
  reconcile_net_state_system.register_server_snapshot(snapshot, server_clock);

  pb::GameClientSingleMessage msg{};
  msg.mutable_snapshot_received()->set_snapshot_id(snapshot.snapshot_id());
  queue_client_message(world, msg);
}

void ecs::add_player_movement_indicator(entt::registry& world,
                                        const pb::PlayerMovement& msg) {
  const char* kLogLabel = "ecs::add_player_movement_indicator";

  pve::MovementIndicatorRenderSystem::add_indicator(
      world, glm::vec2(msg.destination().x(), msg.destination().y()));

  Logger::log(kLogLabel) << "Player movement registered at : "
                         << msg.destination().x() << ", "
                         << msg.destination().y();
}
