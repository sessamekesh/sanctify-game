#ifndef SANCTIFY_GAME_CLIENT_SRC_RENDER_COMMON_CAMERA_UBO_H
#define SANCTIFY_GAME_CLIENT_SRC_RENDER_COMMON_CAMERA_UBO_H

#include <iggpu/ubo_base.h>

#include <glm/glm.hpp>

namespace sanctify::render {

struct CameraCommonVsBufferData {
  glm::mat4 matView;
  glm::mat4 matProj;
};

struct CameraCommonFsBufferData {
  glm::vec3 cameraPos;
};

struct CommonLightingParamsData {
  glm::vec3 lightDirection;
  float ambientCoefficient;
  glm::vec3 lightColor;
  float specularPower;
};

typedef indigo::iggpu::UboBase<CameraCommonVsBufferData> CameraCommonVsUbo;
typedef indigo::iggpu::UboBase<CameraCommonFsBufferData> CameraCommonFsUbo;

}  // namespace sanctify::render

#endif
