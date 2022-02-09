#include <app/systems/locomotion.h>

using namespace sanctify;
using namespace system;
using namespace indigo;
using namespace core;

void ServerLocomotionSystem::handle_player_movement_event(
    const pb::PlayerMovement& action, entt::entity player_entity,
    entt::registry& world) {
  if (!action.has_destination()) {
    return;
  }

  // TODO (sessamekesh): use a navmesh to come up with a better path
  core::PodVector<glm::vec2> waypoints(1);
  waypoints.push_back(
      glm::vec2{action.destination().x(), action.destination().y()});
  world.emplace_or_replace<component::NavWaypointList>(player_entity,
                                                       std::move(waypoints));
}
