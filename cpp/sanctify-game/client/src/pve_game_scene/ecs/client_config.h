#ifndef SANCTIFY_GAME_CLIENT_SRC_PVE_GAME_SCENE_ECS_CLIENT_CONFIG_H
#define SANCTIFY_GAME_CLIENT_SRC_PVE_GAME_SCENE_ECS_CLIENT_CONFIG_H

namespace sanctify {

struct ClientConfigComponent {
  /** How long should the sim clock sync phase take? */
  float simClockSyncTime = 6.f;

  /** During the sim clock sync phase, how often should pings be sent? */
  float simClockSyncPingPhaseTime = 0.25f;
};

}  // namespace sanctify

#endif
