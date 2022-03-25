#ifndef SANCTIFY_GAME_SERVER_APP_PVE_GAME_SERVER_ECS_NETSTATE_COMPONENTS_H
#define SANCTIFY_GAME_SERVER_APP_PVE_GAME_SERVER_ECS_NETSTATE_COMPONENTS_H

#include <igcore/maybe.h>
#include <sanctify-game-common/proto/sanctify-net.pb.h>
#include <util/concurrentqueue.h>

#include <entt/entt.hpp>
#include <functional>
#include <queue>
#include <shared_mutex>

namespace sanctify::ecs::net {

/** Outgoing message queue */
struct PlayerOutgoingMessageQueue {
  PlayerOutgoingMessageQueue() : outgoingActions(256u) {}

  moodycamel::ConcurrentQueue<pb::GameServerSingleMessage> outgoingActions;
};

void queue_single_message(entt::registry& world, entt::entity e,
                          const pb::GameServerSingleMessage& action);
void flush_messages(entt::registry& world, entt::entity e);

}  // namespace sanctify::ecs::net

#endif
