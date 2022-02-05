#ifndef SANCTIFY_GAME_CLIENT_SRC_GAME_SCENE_SYSTEMS_PLAYER_RENDER_SYSTEM_H
#define SANCTIFY_GAME_CLIENT_SRC_GAME_SCENE_SYSTEMS_PLAYER_RENDER_SYSTEM_H

//
// Player Render System
//
// Handles tasks related to updating the input buffers and issuing draw commands
//  for rendering the players
//

#include <igcore/pod_vector.h>
#include <render/solid_animated/solid_animated_geo.h>
#include <render/solid_animated/solid_animated_pipeline.h>

#include <entt/entt.hpp>

namespace sanctify::system {

class PlayerRenderSystem {
 public:
  // Query method like this so that this can potentially be run in a worker
  // thread and joined back later
  indigo::core::PodVector<solid_animated::MatWorldInstanceData>
  get_instance_data(entt::registry& world);
};

}  // namespace sanctify::system

#endif
