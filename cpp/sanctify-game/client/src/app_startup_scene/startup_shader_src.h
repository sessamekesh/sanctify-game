#ifndef SANCTIFY_GAME_CLIENT_SRC_APP_STARTUP_SCENE_STARTUP_SHADER_SRC_H
#define SANCTIFY_GAME_CLIENT_SRC_APP_STARTUP_SCENE_STARTUP_SHADER_SRC_H

#include <glm/glm.hpp>

namespace sanctify {

extern const char* kStartupShaderVsSrc;

struct FrameParams {
  glm::vec2 Dimensions;
  float Time;
};

// Shader is largely inspired by https://www.shadertoy.com/view/WtjBRz
extern const char* kStartupShaderFsSrc;

}  // namespace sanctify

#endif
