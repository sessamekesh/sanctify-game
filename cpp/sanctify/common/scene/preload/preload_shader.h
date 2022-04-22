#ifndef SANCTIFY_COMMON_SCENE_PRELOAD_PRELOAD_SHADER_H
#define SANCTIFY_COMMON_SCENE_PRELOAD_PRELOAD_SHADER_H

#include <glm/vec2.hpp>
#include <string>

namespace sanctify {

class PreloadShader {
 public:
  struct FsParams {
    glm::vec2 viewDimensions;  // Is this needed?
    float t;
    float ballSize;
    float pathRadius;
  };
};

extern const std::string kPreloadShaderVsSrc;
extern const std::string kPreloadShaderFsSrc;

}  // namespace sanctify

#endif
