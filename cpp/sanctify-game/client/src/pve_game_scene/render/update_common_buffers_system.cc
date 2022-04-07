#include "update_common_buffers_system.h"

#include <pve_game_scene/ecs/camera.h>
#include <pve_game_scene/render/common_resources.h>

#include <glm/gtc/matrix_transform.hpp>

using namespace sanctify;
using namespace pve;

void UpdateCommonBuffersSystem::update(entt::registry& world,
                                       const wgpu::Device& device) {
  auto& camera = world.ctx<GameCamera>();
  auto& common_buffers = world.ctx<Common3dGpuBuffers>();
  {
    auto& vert_params = common_buffers.cameraCommonVsUbo.get_mutable();
    vert_params.matView = camera.arenaCamera.mat_view();
    vert_params.matProj =
        glm::perspective(camera.fovy, camera.aspectRatio, 0.1f, 1000.f);
    common_buffers.cameraCommonVsUbo.sync(device);
  }
  {
    auto& frag_params = common_buffers.cameraCommonFsUbo.get_mutable();
    frag_params.cameraPos = camera.arenaCamera.position();
    common_buffers.cameraCommonFsUbo.sync(device);
  }
  {
    // TODO (sessamekesh): Set in a context variable instead of here
    auto& lighting_params = common_buffers.commonLightingUbo.get_mutable();
    lighting_params.ambientCoefficient = 0.3f;
    lighting_params.lightDirection = glm::normalize(glm::vec3(1.f, -3.f, 1.f));
    lighting_params.lightColor = glm::vec3(1.f, 1.f, 1.f);
    lighting_params.specularPower = 50.f;
    common_buffers.commonLightingUbo.sync(device);
  }
}
