#include "transform_3d.h"

#include <glm/gtc/matrix_transform.hpp>

using namespace sanctify::common;

glm::mat4 Transform3dUtil::get_transform(const glm::vec2& map_pos,
                                         float orientation,
                                         const glm::vec3& model_scale) {
  glm::mat4 translated =
      glm::translate(glm::mat4(1.f), glm::vec3(map_pos.x, 0.f, map_pos.y));
  glm::mat4 rotated =
      glm::rotate(translated, orientation, glm::vec3(0.f, 1.f, 0.f));
  glm::mat4 scaled = glm::scale(rotated, model_scale);

  return scaled;
}