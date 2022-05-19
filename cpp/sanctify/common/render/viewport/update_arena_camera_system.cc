#include "update_arena_camera_system.h"

#include <common/render/common/render_components.h>

#include <glm/gtc/matrix_transform.hpp>

using namespace sanctify;
using namespace render;

using namespace indigo;

namespace {

struct CtxPerspectiveParams {
  float lastAspectRatio;
};

const igecs::WorldView::Decl kUpdateDecl =
    igecs::WorldView::Decl()
        .ctx_writes<::CtxPerspectiveParams>()
        .ctx_writes<CtxArenaCameraInputs>()
        .ctx_writes<CtxArenaCamera>()
        .ctx_writes<CtxMainCameraCommonUbos>()
        .ctx_reads<CtxHdrFramebufferParams>()
        .ctx_reads<CtxPlatformObjects>();
}  // namespace

void UpdateArenaCameraSystem::set_camera(indigo::igecs::WorldView* wv,
                                         logic::ArenaCamera camera,
                                         float min_clip, float max_clip,
                                         float fovy) {
  if (wv->ctx_has<CtxArenaCamera>()) {
    auto& ctx = wv->mut_ctx<CtxArenaCamera>();
    ctx.arenaCamera = camera;
    ctx.nearClippingPlane = min_clip;
    ctx.farClippingPlane = max_clip;
    ctx.fovy = fovy;
  } else {
    wv->attach_ctx<CtxArenaCamera>(camera, min_clip, max_clip, fovy);
  }

  wv->mut_ctx_or_set<CtxArenaCameraInputs>(glm::vec3{}, 0.f) =
      CtxArenaCameraInputs{glm::vec3{}, 0.f};
}

void UpdateArenaCameraSystem::set_lighting(indigo::igecs::WorldView* wv,
                                           CommonLightingParamsData data) {
  wv->mut_ctx<CtxMainCameraCommonUbos>().lightingUbo.get_mutable() = data;
}

const igecs::WorldView::Decl& UpdateArenaCameraSystem::update_decl() {
  return ::kUpdateDecl;
}

void UpdateArenaCameraSystem::update(indigo::igecs::WorldView* wv) {
  auto& ubos = wv->mut_ctx<CtxMainCameraCommonUbos>();
  auto& ctx_camera = wv->mut_ctx<CtxArenaCamera>();
  auto& camera = ctx_camera.arenaCamera;
  auto& framebuffer_params = wv->ctx<CtxHdrFramebufferParams>();
  auto& platform = wv->ctx<CtxPlatformObjects>();
  auto& ctx_perspective = wv->mut_ctx_or_set<::CtxPerspectiveParams>(-1.f);
  uint32_t vp_width = platform.viewportWidth;
  uint32_t vp_height = platform.viewportHeight;

  auto& updates = wv->mut_ctx<CtxArenaCameraInputs>();

  float aspect_ratio = (float)vp_width / (float)vp_height;
  if (updates.lookAtAdjustment != glm::vec3(0.f, 0.f, 0.f) ||
      updates.radiusAdjustment != 0.f ||
      ubos.cameraVsUbo.get_immutable().matView[3][3] == 0.f ||
      aspect_ratio != ctx_perspective.lastAspectRatio) {
    camera.set_radius(camera.radius() + updates.radiusAdjustment);
    camera.set_look_at(camera.look_at() + updates.lookAtAdjustment);

    updates.lookAtAdjustment = glm::vec3(0.f, 0.f, 0.f);
    updates.radiusAdjustment = 0.f;

    auto& vs_params = ubos.cameraVsUbo.get_mutable();
    auto& fs_params = ubos.cameraFsUbo.get_mutable();

    vs_params.matView = camera.mat_view();
    vs_params.matProj = glm::perspective(ctx_camera.fovy, aspect_ratio,
                                         ctx_camera.nearClippingPlane,
                                         ctx_camera.farClippingPlane);
    fs_params.cameraPos = camera.position();

    ctx_perspective.lastAspectRatio = aspect_ratio;
  }

  ubos.lightingUbo.sync(platform.device);
  ubos.cameraFsUbo.sync(platform.device);
  ubos.cameraVsUbo.sync(platform.device);
}
