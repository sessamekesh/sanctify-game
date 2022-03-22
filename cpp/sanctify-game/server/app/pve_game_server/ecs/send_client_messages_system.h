#ifndef SANCTIFY_GAME_SERVER_APP_PVE_GAME_SERVER_SEND_CLIENT_MESSAGES_SYSTEM_H
#define SANCTIFY_GAME_SERVER_APP_PVE_GAME_SERVER_SEND_CLIENT_MESSAGES_SYSTEM_H

#include <sanctify-game-common/proto/sanctify-net.pb.h>
#include <util/types.h>

#include <entt/entt.hpp>

namespace sanctify::system {

class QueueClientMessagesSystem {
 public:
  void update(entt::registry& world, float sim_time);
};

class SendClientMessagesSystem {
 public:
  void update(entt::registry& world,
              const std::function<void(PlayerId, pb::GameServerMessage)>& cb);
};

}  // namespace sanctify::system

#endif
