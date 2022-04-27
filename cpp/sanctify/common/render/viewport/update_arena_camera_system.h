#ifndef SANCTIFY_COMMON_RENDER_VIEWPORT_UPDATE_ARENA_CAMERA_SYSTEM_H
#define SANCTIFY_COMMON_RENDER_VIEWPORT_UPDATE_ARENA_CAMERA_SYSTEM_H

#include <common/logic/viewport/arena_camera.h>
#include <common/render/common/camera_ubos.h>
#include <igecs/world_view.h>
#include <webgpu/webgpu_cpp.h>

namespace sanctify::render {

struct CtxArenaCameraInputs {
  glm::vec3 lookAtAdjustment;
  float radiusAdjustment;
};

struct CtxArenaCamera {
  logic::ArenaCamera arenaCamera;
  float nearClippingPlane;
  float farClippingPlane;
  float fovy;
};

class UpdateArenaCameraSystem {
 public:
  static void set_camera(indigo::igecs::WorldView* wv,
                         logic::ArenaCamera camera, float min_clip,
                         float max_clip, float fovy);
  static void set_lighting(indigo::igecs::WorldView* wv,
                           CommonLightingParamsData lighting_data);

  static const indigo::igecs::WorldView::Decl& update_decl();

  static void update(indigo::igecs::WorldView* wv);
};

}  // namespace sanctify::render

#endif
