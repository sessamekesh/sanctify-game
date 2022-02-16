#include <util/recast_params.h>

using namespace mapeditor;

namespace {
const float kDefaultCellSize = 0.25f;
const glm::vec3 kDefaultMinBb = glm::vec3(-50.f, -1.f, -50.f);
const glm::vec3 kDefaultMaxBb = glm::vec3(50.f, 20.f, 50.f);
}  // namespace

RecastParams::RecastParams() {}

float RecastParams::cell_size() const {
  if (recast_nav_mesh_action_proto_.cell_size() != 0.f) {
    return recast_nav_mesh_action_proto_.cell_size();
  }

  return ::kDefaultCellSize;
}

void RecastParams::cell_size(float value) {
  recast_nav_mesh_action_proto_.set_cell_size(value);
}

glm::vec3 RecastParams::min_bb() const {
  if (recast_nav_mesh_action_proto_.has_bb_override()) {
    return glm::vec3{recast_nav_mesh_action_proto_.bb_override().min_x(),
                     recast_nav_mesh_action_proto_.bb_override().min_y(),
                     recast_nav_mesh_action_proto_.bb_override().min_z()};
  }

  return ::kDefaultMinBb;
}

glm::vec3 RecastParams::max_bb() const {
  if (recast_nav_mesh_action_proto_.has_bb_override()) {
    return glm::vec3{recast_nav_mesh_action_proto_.bb_override().max_x(),
                     recast_nav_mesh_action_proto_.bb_override().max_y(),
                     recast_nav_mesh_action_proto_.bb_override().max_z()};
  }

  return ::kDefaultMaxBb;
}