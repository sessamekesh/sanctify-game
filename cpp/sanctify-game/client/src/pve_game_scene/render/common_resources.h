#ifndef SANCTIFY_GAME_CLIENT_PVE_GAME_SCENE_RENDER_COMMON_RESOURCES_H
#define SANCTIFY_GAME_CLIENT_PVE_GAME_SCENE_RENDER_COMMON_RESOURCES_H

#include <iggpu/ubo_base.h>
#include <render/common/camera_ubo.h>
#include <webgpu/webgpu_cpp.h>

#include <entt/entt.hpp>

namespace sanctify::pve {

/**
 * Common GPU buffers that are used in all / almost all 3D presentation shaders
 */
struct Common3dGpuBuffers {
  indigo::iggpu::UboBase<render::CameraCommonVsBufferData>
      CommonCameraVertParams;
  indigo::iggpu::UboBase<render::CameraCommonFsBufferData>
      CommonCameraFragParams;
  indigo::iggpu::UboBase<render::CommonLightingParamsData> CommonLightingParams;
};

/**
 * Final pass render target / depth buffer target (set per frame). Color
 * buffer is presented to the player at the end of the frame, and the depth
 * stencil buffer should be used to make determinations about what is drawn
 * to that final buffer.
 */
struct FinalRenderTargetBuffers {
  wgpu::TextureFormat colorTargetFormat;
  wgpu::TextureView colorTarget;
  wgpu::TextureView depthStencilView;
};

void set_frame_render_targets(entt::registry& world,
                              wgpu::TextureView color_target,
                              wgpu::TextureFormat color_target_format,
                              wgpu::TextureView depth_stencil_view);

}  // namespace sanctify::pve

#endif
