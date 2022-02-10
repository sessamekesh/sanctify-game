#include <game_scene/systems/player_move_indicator_render_system.h>

using namespace sanctify;

using namespace indigo;
using namespace core;

PlayerMoveIndicatorRenderSystem::PlayerMoveIndicatorRenderSystem(
    glm::vec3 start_color, glm::vec3 end_color, float lifetime,
    glm::vec3 start_scale, glm::vec3 end_scale)
    : instances_(4),
      movement_indicator_data_(4),
      start_color_(start_color),
      end_color_(end_color),
      lifetime_(lifetime),
      start_scale_(start_scale),
      end_scale_(end_scale) {}

void PlayerMoveIndicatorRenderSystem::add_at_location(glm::vec3 location) {
  debug_geo::InstanceData data{};
  data.MatWorld = glm::translate(glm::mat4(1.f), location);
  data.MatWorld = glm::scale(data.MatWorld, start_scale_);
  data.ObjectColor = start_color_;
  instances_.push_back(data);

  movement_indicator_data_.push_back(MovementIndicator{lifetime_, location});
}

void PlayerMoveIndicatorRenderSystem::update(float dt) {
  for (int i = 0; i < instances_.size(); i++) {
    movement_indicator_data_[i].t -= dt;

    if (movement_indicator_data_[i].t < 0.f) {
      movement_indicator_data_.delete_at(i, false);
      instances_.delete_at(i, false);
      i--;
      continue;
    }

    float t = 1.f - movement_indicator_data_[i].t / lifetime_;

    glm::vec3 scale = t * end_scale_ + (1.f - t) * start_scale_;
    glm::vec3 color = t * end_color_ + (1.f - t) * start_color_;

    auto& data = instances_[i];

    data.MatWorld =
        glm::translate(glm::mat4(1.f), movement_indicator_data_[i].location);
    data.MatWorld = glm::scale(data.MatWorld, scale);
    data.ObjectColor = color;
  }
}

const PodVector<debug_geo::InstanceData>& PlayerMoveIndicatorRenderSystem::get()
    const {
  return instances_;
}
