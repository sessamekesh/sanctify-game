#include <DetourNavMeshQuery.h>
#include <app/systems/locomotion.h>
#include <igcore/log.h>

using namespace sanctify;
using namespace system;
using namespace indigo;
using namespace core;

namespace {
const char* kLogLabel = "ServerLocomotionSystem";
}

Maybe<glm::vec2> ServerLocomotionSystem::handle_player_movement_event(
    const pb::PlayerMovement& action, entt::entity player_entity,
    entt::registry& world, const indigo::nav::DetourNavmesh& navmesh) {
  if (!action.has_destination()) {
    return empty_maybe{};
  }

  // TODO (sessamekesh): use a navmesh to come up with a better path
  auto* loc = world.try_get<component::MapLocation>(player_entity);
  if (loc == nullptr) {
    // Cannot assign player a path, since they do not exist on the map!
    return empty_maybe{};
  }

  // TODO (sessamekesh) - pathfinding query should be allocated once, and
  //  this system should have a consumed reference to the navmesh?
  dtNavMeshQuery* pathfinding_query = dtAllocNavMeshQuery();
  dtQueryFilter filter;
  glm::vec3 center(loc->XZ.x, 0.f, loc->XZ.y);
  glm::vec3 half_extents(20.f, 2.f, 20.f);
  dtPolyRef start_ref{};
  glm::vec3 start_pt{};

  glm::vec3 requested_destination(action.destination().x(), 0.f,
                                  action.destination().y());
  dtPolyRef end_ref{};
  glm::vec3 actual_dest{};

  pathfinding_query->init(navmesh.raw(), 2048);

  dtStatus start_status = pathfinding_query->findNearestPoly(
      &center.x, &half_extents.x, &filter, &start_ref, &start_pt.x);
  dtStatus end_status = pathfinding_query->findNearestPoly(
      &requested_destination.x, &half_extents.x, &filter, &end_ref,
      &actual_dest.x);

  if (dtStatusFailed(start_status) || dtStatusFailed(end_status)) {
    dtFreeNavMeshQuery(pathfinding_query);
    return empty_maybe{};
  }

  const int kMaxPolys = 64;
  dtPolyRef path[kMaxPolys]{};
  int poly_count = 0;
  dtStatus pathfind_status = pathfinding_query->findPath(
      start_ref, end_ref, &start_pt.x, &actual_dest.x, &filter, path,
      &poly_count, kMaxPolys);
  if (dtStatusFailed(pathfind_status) || poly_count == 0) {
    dtFreeNavMeshQuery(pathfinding_query);
    return empty_maybe{};
  }

  glm::vec3 straight_path[kMaxPolys]{};
  int num_path_points = 0;
  // TODO (sessamekesh): For some reason this isn't actually straight
  dtStatus straight_path_status = pathfinding_query->findStraightPath(
      &start_pt.x, &actual_dest.x, path, poly_count, &straight_path[0].x,
      nullptr, nullptr, &num_path_points, kMaxPolys);
  if (dtStatusFailed(straight_path_status) || num_path_points == 0) {
    dtFreeNavMeshQuery(pathfinding_query);
    return empty_maybe{};
  }

  core::PodVector<glm::vec2> waypoints(num_path_points + 1);
  for (int i = 0; i < num_path_points; i++) {
    waypoints.push_back(glm::vec2(straight_path[i].x, straight_path[i].z));
  }
  waypoints.push_back(glm::vec2(actual_dest.x, actual_dest.z));

  auto l = Logger::log(kLogLabel);
  l << "Waypoint from (" << loc->XZ.x << ", " << loc->XZ.y << ") to ("
    << actual_dest.x << ", " << actual_dest.z << ")\n";
  for (int i = 0; i < waypoints.size(); i++) {
    l << "-- " << waypoints[i].x << ", " << waypoints[i].y << "\n";
  }

  world.emplace_or_replace<component::NavWaypointList>(player_entity,
                                                       std::move(waypoints));

  dtFreeNavMeshQuery(pathfinding_query);

  return glm::vec2(actual_dest.x, actual_dest.z);
}
