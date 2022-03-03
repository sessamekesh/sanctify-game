#ifndef SANCTIFY_GAME_CLIENT_SRC_ECS_SYSTEMS_DESTROY_CHILDREN_SYSTEM_H
#define SANCTIFY_GAME_CLIENT_SRC_ECS_SYSTEMS_DESTROY_CHILDREN_SYSTEM_H

#include <entt/entt.hpp>

namespace sanctify::ecs {

class DestroyChildrenSystem {
 public:
  void run(entt::registry& world);
};

}  // namespace sanctify::ecs

#endif
