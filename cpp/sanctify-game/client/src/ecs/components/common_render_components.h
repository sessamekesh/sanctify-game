#ifndef SANCTIFY_GAME_CLIENT_SRC_ECS_COMPONENTS_COMMON_RENDER_COMPONENTS_H
#define SANCTIFY_GAME_CLIENT_SRC_ECS_COMPONENTS_COMMON_RENDER_COMPONENTS_H

#include <glm/glm.hpp>

/**
 * Components that are common to many types of rendering
 */

namespace sanctify::ecs {

struct MatWorldComponent {
  glm::mat4 matWorld;
};

}  // namespace sanctify::ecs

#endif
