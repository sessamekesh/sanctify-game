#include <igcore/log.h>
#include <pve_game_scene/ecs/client_config.h>
#include <pve_game_scene/ecs/sim_time_sync_system.h>
#include <pve_game_scene/ecs/utils.h>

using namespace sanctify;
using namespace indigo;
using namespace core;

namespace {
const char* kLogLabel = "SimTimeSyncSystem";
}

SimTimeSyncSystem::LoadTimeDeltaComponent::LoadTimeDeltaComponent(
    float load_time)
    : loadTimeLeft(load_time),
      nextPingTime(0.f),
      nextPingId(0xFF0000DD),
      hasPerformedReconcile(false) {}

void SimTimeSyncSystem::loading_update(entt::registry& world, entt::entity e,
                                       float dt) {
  auto& client_config = world.get<ClientConfigComponent>(e);

  auto& ltd_component = world.get_or_emplace<LoadTimeDeltaComponent>(
      e, client_config.simClockSyncTime);

  auto sim_time = ecs::sim_clock_time(world, e);

  if (ltd_component.loadTimeLeft <= 0.f &&
      ltd_component.timeDeltas.size() > 10) {
    if (ltd_component.hasPerformedReconcile) return;

    float sum = 0.f;
    for (int i = 0; i < ltd_component.timeDeltas.size(); i++) {
      sum += ltd_component.timeDeltas[i];
    }
    float adjust_time = sum / (float)ltd_component.timeDeltas.size();

    Logger::log(kLogLabel) << "Adjusting server clock by " << adjust_time;

    ecs::sim_clock_time(world, e) += adjust_time;
    ltd_component.timeDeltas = PodVector<float>(1);
    ltd_component.hasPerformedReconcile = true;
    return;
  }

  ltd_component.loadTimeLeft -= dt;

  ltd_component.nextPingTime -= dt;
  if (ltd_component.nextPingTime <= 0.f) {
    ltd_component.nextPingTime = client_config.simClockSyncPingPhaseTime;

    pb::GameClientSingleMessage msg{};
    auto* ping = msg.mutable_client_ping();
    ping->set_is_ready(false);
    ping->set_local_time(sim_time);
    ping->set_ping_id(ltd_component.nextPingId++);
    ltd_component.expectedPingIds.push_back(ping->ping_id());
    ecs::queue_client_message(world, e, msg);
  }
}

void SimTimeSyncSystem::handle_pong(entt::registry& world, entt::entity e,
                                    const pb::ServerPong& msg) {
  auto& client_config = world.get<ClientConfigComponent>(e);
  auto& ltd_component = world.get_or_emplace<LoadTimeDeltaComponent>(
      e, client_config.simClockSyncTime);
  float sim_time = ecs::sim_clock_time(world, e);

  if (ltd_component.expectedPingIds.contains(msg.ping_id())) {
    ltd_component.expectedPingIds.erase(msg.ping_id());
    ltd_component.timeDeltas.push_back(msg.sim_time() - sim_time);
    ltd_component.expectedPingIds.erase(msg.ping_id(), false);
  }
}

bool SimTimeSyncSystem::is_done_loading(entt::registry& world, entt::entity e) {
  auto& client_config = world.get<ClientConfigComponent>(e);

  auto& ltd_component = world.get_or_emplace<LoadTimeDeltaComponent>(
      e, client_config.simClockSyncTime);

  return ltd_component.hasPerformedReconcile;
}
