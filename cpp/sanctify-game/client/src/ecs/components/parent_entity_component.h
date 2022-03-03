#ifndef SANCTIFY_GAME_CLIENT_SRC_ESC_COMPONENTS_PARENT_ENTITY_COMPONENT_H
#define SANCTIFY_GAME_CLIENT_SRC_ESC_COMPONENTS_PARENT_ENTITY_COMPONENT_H

#include <entt/entt.hpp>

namespace sanctify::ecs {

struct ParentEntityComponent {
  entt::entity parentEntity;
};

}  // namespace sanctify::ecs

#endif
