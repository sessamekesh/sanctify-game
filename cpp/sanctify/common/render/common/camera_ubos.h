#ifndef SANCTIFY_COMMON_RENDER_COMMON_CAMERA_UBOS_H
#define SANCTIFY_COMMON_RENDER_COMMON_CAMERA_UBOS_H

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
typedef indigo::iggpu::UboBase<CommonLightingParamsData> CommonLightingUbo;

}  // namespace sanctify::render

#endif
