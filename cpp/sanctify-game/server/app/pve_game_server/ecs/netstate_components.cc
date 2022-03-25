#include <app/pve_game_server/ecs/context_components.h>
#include <app/pve_game_server/ecs/netstate_components.h>
#include <app/pve_game_server/ecs/player_context_components.h>
#include <igcore/log.h>

using namespace sanctify;
using namespace ecs;
using namespace indigo;
using namespace core;

namespace {
const char* kLogLabel = "netstate_components.cc";
}

void net::queue_single_message(entt::registry& world, entt::entity e,
                               const pb::GameServerSingleMessage& action) {
  const auto& server_config = world.ctx<ecs::GServerConfig>();
  auto& msg_queue =
      world.get<net::PlayerOutgoingMessageQueue>(e).outgoingActions;

  if (msg_queue.size_approx() >= server_config.maxQueuedServerActions) {
    const auto& player_attribs = world.get<ecs::PlayerSystemAttributes>(e);
    Logger::log(kLogLabel) << "queue_single_message: queue limit reached, "
                              "discarding action (player "
                           << player_attribs.playerId.Id << ", queue limit "
                           << server_config.maxQueuedServerActions << ")";
    pb::GameServerSingleMessage msg;
    msg_queue.try_dequeue(msg);
  }
  msg_queue.enqueue(action);
}

void net::flush_messages(entt::registry& world, entt::entity e) {
  const auto& send_message_function =
      world.ctx<ecs::GEngineCallbacks>().sendNetMessageCb;
  const auto& server_config = world.ctx<ecs::GServerConfig>();
  const PlayerId& player_id =
      world.get<ecs::PlayerSystemAttributes>(e).playerId;
  const auto& netstate = world.get<ecs::PlayerConnectionState>(e).netState;

  auto& msg_queue =
      world.get<net::PlayerOutgoingMessageQueue>(e).outgoingActions;

  /** High but bounded message loop limit to prevent infinite loop */
  for (int i = 0; i < 500; i++) {
    pb::GameServerMessage msg{};
    auto* action_list = msg.mutable_actions_list();

    pb::GameServerSingleMessage action{};
    for (int i = 0; i < server_config.maxActionsPerMessage; i++) {
      if (!msg_queue.try_dequeue(action)) {
        break;
      }
      *action_list->add_messages() = action;
    }

    if (action_list->messages_size() == 0) {
      break;
    }

    if (netstate == ecs::PlayerConnectionState::NetState::Healthy) {
      send_message_function(player_id, std::move(msg));
    }
  }
}
