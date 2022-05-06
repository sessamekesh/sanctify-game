#ifndef SANCTIFY_COMMON_RENDER_COMMON_RENDER_COMPONENTS_H
#define SANCTIFY_COMMON_RENDER_COMMON_RENDER_COMPONENTS_H

#include <webgpu/webgpu_cpp.h>

#include <glm/glm.hpp>

#include "camera_ubos.h"

namespace sanctify::render {

struct MatWorldComponent {
  glm::mat4 matWorld;
};

struct CtxHdrFramebufferParams {
  wgpu::TextureFormat format;
  uint32_t width;
  uint32_t height;
};

struct CtxPlatformObjects {
  wgpu::Device device;
  wgpu::TextureFormat swapChainFormat;
  wgpu::TextureView swapChainBackbuffer;
  uint32_t viewportWidth;
  uint32_t viewportHeight;
};

struct CtxMainCameraCommonUbos {
  CameraCommonVsUbo cameraVsUbo;
  CameraCommonFsUbo cameraFsUbo;
  CommonLightingUbo lightingUbo;
};

}  // namespace sanctify::render

#endif
