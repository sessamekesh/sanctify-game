#ifndef SANCTIFY_GAME_SERVER_APP_PVE_GAME_SERVER_ECS_PLAYER_CONTEXT_COMPONENTS_H
#define SANCTIFY_GAME_SERVER_APP_PVE_GAME_SERVER_ECS_PLAYER_CONTEXT_COMPONENTS_H

#include <sanctify-game-common/proto/api-objects.pb.h>
#include <util/types.h>

#include <entt/entt.hpp>
#include <string>

namespace sanctify::ecs {

/** Attributes of this player */
struct PlayerSystemAttributes {
  PlayerId playerId;
  std::string displayName;
  std::string tagline;
};

/** Connection state of the player */
struct PlayerConnectionState {
  enum class NetState {
    /** Player is in a healthy net state and can be reached */
    Healthy,

    /** Player is not in a healthy net state but has not yet disconnected */
    Unhealthy,

    /** Player connection has terminated, and must re-connect */
    Disconnected,
  };

  NetState netState;

  /**
   * True if the player client has passed its loading stage and is ready for
   * world state updates.
   */
  bool isReady;
};

/**
 * Create the absolute minimum boilerplate required for any attached player. Raw
 *  details about the player, including their system ID, display name, tagline,
 *  etc., but not any gameplay/netsync details - those get added in later
 *  systems.
 */
void bootstrap_server_player(entt::registry& world, entt::entity e,
                             const pb::GameServerPlayerDescription server_desc);

const bool& is_ready(entt::registry& world, entt::entity e);

class PlayerUtil {
 public:
  static PlayerId player_id(entt::registry& world, entt::entity e);
};

}  // namespace sanctify::ecs

#endif
