#ifndef SANCTIFY_GAME_CLIENT_SRC_PVE_GAME_SCENE_ECS_CLIENT_CONFIG_H
#define SANCTIFY_GAME_CLIENT_SRC_PVE_GAME_SCENE_ECS_CLIENT_CONFIG_H

#include <glm/glm.hpp>

namespace sanctify {

struct ClientConfigComponent {
  /** How long should the sim clock sync phase take? */
  float simClockSyncTime = 6.f;

  /** During the sim clock sync phase, how often should pings be sent? */
  float simClockSyncPingPhaseTime = 0.25f;

  /** Default arena camera parameters */
  glm::vec3 cameraDefaultLookAt = glm::vec3(0.f, 0.f, 0.f);
  float cameraDefaultTilt = glm::radians(45.f);
  float cameraDefaultSpin = glm::radians(30.f);
  float cameraDefaultRadius = 85.f;
  float cameraDefaultMovementSpeed = 0.8f;
  float cameraDefaultFovy = glm::radians(40.f);

  /** Movement indicator render params */
  struct {
    float startScale = 1.f;
    float endScale = 0.2f;
    float lifetimeSeconds = 0.4f;
    glm::vec3 startColor = glm::vec3(0.4f, 0.4f, 1.f);
    glm::vec3 endColor = glm::vec3(0.f, 0.f, 0.6f);
  } moveIndicatorRenderParams;
};

}  // namespace sanctify

#endif
