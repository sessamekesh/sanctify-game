#ifndef SANCTIFY_COMMON_UTIL_TRANSFORM_3D_H
#define SANCTIFY_COMMON_UTIL_TRANSFORM_3D_H

#include <glm/glm.hpp>

namespace sanctify::common {

struct Transform3dUtil {
  static glm::mat4 get_transform(const glm::vec2& map_pos, float orientation,
                                 const glm::vec3& model_scale);
};

}  // namespace sanctify::common

#endif
