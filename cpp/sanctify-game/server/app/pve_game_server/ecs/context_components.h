#ifndef SANCTIFY_GAME_SERVER_APP_PVE_GAME_SERVER_ECS_CONTEXT_COMPONENTS_H
#define SANCTIFY_GAME_SERVER_APP_PVE_GAME_SERVER_ECS_CONTEXT_COMPONENTS_H

#include <igcore/maybe.h>
#include <igcore/pod_vector.h>
#include <sanctify-game-common/proto/api-objects.pb.h>
#include <sanctify-game-common/proto/sanctify-net.pb.h>
#include <util/types.h>

#include <entt/entt.hpp>
#include <functional>
#include <map>
#include <set>

namespace sanctify::ecs {

/**
 * Global component representing current timing data
 */
struct GClock {
  // How much time has elapsed since the beginning of the sim?
  float simTime;

  // How much time should elapse in this game tick?
  float frameTime;
};

const float& sim_time(entt::registry& world);
const float& frame_time(entt::registry& world);
void tick(entt::registry& world, float dt);

/**
 * Attached players
 */
struct GExpectedPlayers {
  /** Use this for regular servers (5 player games) */
  std::vector<pb::GameServerPlayerDescription> playerDescriptions;

  /** Quick access for player entity */
  std::map<PlayerId, entt::entity> entityMap;
};

void add_expected_player(entt::registry& world,
                         const pb::GameServerPlayerDescription& player_desc);

indigo::core::Maybe<entt::entity> get_player_entity(entt::registry& world,
                                                    const PlayerId& player_id);

/**
 * Game engine callbacks
 */
struct GEngineCallbacks {
  std::function<void(PlayerId, pb::GameServerMessage)> sendNetMessageCb;
};

void bootstrap_game_engine_callbacks(
    entt::registry& world,
    std::function<void(PlayerId, pb::GameServerMessage)> send_net_message_cb);

/**
 * Server configuration
 */
struct GServerConfig {
  /** Maximum number of outgoing messages in a single server message */
  uint32_t maxActionsPerMessage;

  /**
   * Maximum number of incoming client messages from a single client (old
   * messages will be discarded)
   */
  uint32_t maxQueuedClientMessages;

  /**
   * Maximum number of outgoing server actions (old messages will be discarded)
   */
  uint32_t maxQueuedServerActions;
};

void bootstrap_server_config(entt::registry& world,
                             uint32_t max_actions_per_message,
                             uint32_t max_queued_client_messages,
                             uint32_t max_queued_server_actions);

}  // namespace sanctify::ecs

#endif
