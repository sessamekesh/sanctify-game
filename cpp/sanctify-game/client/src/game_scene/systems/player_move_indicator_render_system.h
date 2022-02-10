#ifndef SANCTIFY_GAME_CLIENT_SRC_GAME_SCENE_SYSTEMS_PLAYER_MOVE_INDICATOR_RENDER_SYSTEM_H
#define SANCTIFY_GAME_CLIENT_SRC_GAME_SCENE_SYSTEMS_PLAYER_MOVE_INDICATOR_RENDER_SYSTEM_H

//
// Player movement indicator render system
//
// Maintains the state of all the player location indicators
//

#include <igcore/pod_vector.h>
#include <render/debug_geo/debug_geo.h>

#include <glm/glm.hpp>

namespace sanctify {

class PlayerMoveIndicatorRenderSystem {
 public:
  struct MovementIndicator {
    float t;
    glm::vec3 location;
  };

  PlayerMoveIndicatorRenderSystem(glm::vec3 start_color, glm::vec3 end_color,
                                  float lifetime, glm::vec3 start_scale,
                                  glm::vec3 end_scale);

  void add_at_location(glm::vec3 location);
  void update(float dt);

  const indigo::core::PodVector<debug_geo::InstanceData>& get() const;

 private:
  indigo::core::PodVector<debug_geo::InstanceData> instances_;
  indigo::core::PodVector<MovementIndicator> movement_indicator_data_;

  glm::vec3 start_color_;
  glm::vec3 end_color_;
  float lifetime_;
  glm::vec3 start_scale_;
  glm::vec3 end_scale_;
};

}  // namespace sanctify

#endif
