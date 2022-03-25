#ifndef SANCTIFY_GAME_SERVER_APP_PVE_GAME_SERVER_SEND_CLIENT_MESSAGES_SYSTEM_H
#define SANCTIFY_GAME_SERVER_APP_PVE_GAME_SERVER_SEND_CLIENT_MESSAGES_SYSTEM_H

#include <sanctify-game-common/proto/sanctify-net.pb.h>
#include <util/types.h>

#include <entt/entt.hpp>

namespace sanctify::ecs {

struct PlayerNetStateComponent {
  float lastMessageReceivedTime;
  float lastSnapshotSentTime;
  float lastDiffSentTime;

  uint32_t lastAckedSnapshotId;
};

class QueueClientMessagesSystem {
 public:
  void update(entt::registry& world, float sim_time);
};

}  // namespace sanctify::ecs

#endif
