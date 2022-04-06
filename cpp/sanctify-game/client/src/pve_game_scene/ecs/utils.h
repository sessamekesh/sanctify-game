#ifndef SANCTIFY_GAME_CLIENT_SRC_PVE_GAME_SCENE_ECS_UTILS_H
#define SANCTIFY_GAME_CLIENT_SRC_PVE_GAME_SCENE_ECS_UTILS_H

#include <netclient/net_client.h>
#include <sanctify-game-common/proto/sanctify-net.pb.h>

#include <entt/entt.hpp>

namespace sanctify::ecs {

float& sim_clock_time(entt::registry& world);

void queue_client_message(entt::registry& world,
                          pb::GameClientSingleMessage msg);

void flush_message_queue(entt::registry& world,
                         std::shared_ptr<NetClient> net_client,
                         uint32_t max_message_count = 26);

void cache_snapshot_diff(entt::registry& world,
                         const pb::GameSnapshotDiff& diff);

void cache_snapshot_full(entt::registry& world,
                         const pb::GameSnapshotFull& diff);

void add_player_movement_indicator(entt::registry& world,
                                   const pb::PlayerMovement& msg);

}  // namespace sanctify::ecs

#endif
