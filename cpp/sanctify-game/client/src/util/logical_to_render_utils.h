#ifndef SANCTIFY_GAME_CLIENT_SRC_UTIL_LOGICAL_TO_RENDER_UTILS_H
#define SANCTIFY_GAME_CLIENT_SRC_UTIL_LOGICAL_TO_RENDER_UTILS_H

#include <glm/glm.hpp>

namespace sanctify::ltr_utils {

glm::mat4 get_transform(const glm::vec2& map_pos, float orientation,
                        const glm::vec3& model_scale);

}

#endif
