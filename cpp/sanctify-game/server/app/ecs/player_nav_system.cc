#include <DetourNavMeshQuery.h>
#include <app/ecs/player_nav_system.h>
#include <igcore/log.h>
#include <sanctify-game-common/gameplay/locomotion_components.h>
#include <sanctify-game-common/gameplay/player_definition_components.h>

using namespace sanctify;
using namespace indigo;
using namespace core;

namespace {
const char* kLogLabel = "PlayerNavSystem";
}

void component::PlayerNavRequestComponent::attach_on(entt::registry& world,
                                                     entt::entity e,
                                                     glm::vec2 pos) {
  world.emplace_or_replace<component::PlayerNavRequestComponent>(e, pos);
}

void system::PlayerNavSystem::update(entt::registry& world,
                                     const nav::DetourNavmesh& navmesh) {
  auto view = world.view<const component::MapLocation,
                         component::PlayerNavRequestComponent,
                         component::BasicPlayerComponent>();

  for (auto [e, map_location, nav_request] : view.each()) {
    // TODO (sessamekesh): consider saving off previously allocated query?
    dtNavMeshQuery* dt_query = dtAllocNavMeshQuery();
    dtQueryFilter filter;

    glm::vec3 half_extents(20.f, 2.f, 20.f);
    dtPolyRef start_ref{};
    dtPolyRef end_ref{};

    glm::vec3 current_player_pos(map_location.XZ.x, 0.f, map_location.XZ.y);
    glm::vec3 start_pos{};
    glm::vec3 requested_destination(nav_request.requestPosition.x, 0.f,
                                    nav_request.requestPosition.y);
    glm::vec3 actual_dest{};

    dt_query->init(navmesh.raw(), 512);

    dtStatus start_status =
        dt_query->findNearestPoly(&current_player_pos.x, &half_extents.x,
                                  &filter, &start_ref, &start_pos.x);
    dtStatus end_status =
        dt_query->findNearestPoly(&requested_destination.x, &half_extents.x,
                                  &filter, &end_ref, &actual_dest.x);

    if (dtStatusFailed(start_status) || dtStatusFailed(end_status)) {
      dtFreeNavMeshQuery(dt_query);
      Logger::log(kLogLabel)
          << "Failed to find start and/or end pos from ["
          << current_player_pos.x << ", " << current_player_pos.y << ", "
          << current_player_pos.z << "] to [" << requested_destination.x << ", "
          << requested_destination.y << ", " << requested_destination.z << "]";
      continue;
    }

    const int kMaxPolys = 64;
    dtPolyRef path[kMaxPolys]{};
    int poly_count = 0;
    dtStatus pathfind_status =
        dt_query->findPath(start_ref, end_ref, &start_pos.x, &actual_dest.x,
                           &filter, path, &poly_count, kMaxPolys);

    if (dtStatusFailed(pathfind_status) || poly_count == 0) {
      dtFreeNavMeshQuery(dt_query);
      Logger::log(kLogLabel)
          << "Failed to find path polys from [" << start_pos.x << ", "
          << start_pos.y << ", " << start_pos.z << "] to [" << actual_dest.x
          << ", " << actual_dest.y << ", " << actual_dest.z << "]";
      continue;
    }

    glm::vec3 straight_path[kMaxPolys]{};
    int num_path_points = 0;

    dtStatus straight_path_status = dt_query->findStraightPath(
        &start_pos.x, &actual_dest.x, path, poly_count, &straight_path[0].x,
        nullptr, nullptr, &num_path_points, kMaxPolys);
    if (dtStatusFailed(straight_path_status) || num_path_points == 0u) {
      dtFreeNavMeshQuery(dt_query);
      Logger::log(kLogLabel)
          << "Failed to find straight path from [" << start_pos.x << ", "
          << start_pos.y << ", " << start_pos.z << "] to [" << actual_dest.x
          << ", " << actual_dest.y << ", " << actual_dest.z << "]";
      continue;
    }

    core::PodVector<glm::vec2> waypoints(num_path_points);
    for (int i = 0; i < num_path_points; i++) {
      waypoints.push_back(glm::vec2(straight_path[i].x, straight_path[i].z));
    }

    // TODO (sessamekesh): Utilities for adding player ID here
    Logger::log(kLogLabel) << "Created new nav point for player (player ID)";

    world.emplace_or_replace<component::NavWaypointList>(e,
                                                         std::move(waypoints));
    world.remove<component::PlayerNavRequestComponent>(e);

    dtFreeNavMeshQuery(dt_query);
  }
}
