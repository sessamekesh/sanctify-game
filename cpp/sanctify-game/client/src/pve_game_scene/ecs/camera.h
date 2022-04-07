#ifndef SANCTIFY_GAME_CLIENT_SRC_PVE_GAME_SCENE_ECS_CAMERA_H
#define SANCTIFY_GAME_CLIENT_SRC_PVE_GAME_SCENE_ECS_CAMERA_H

#include <io/arena_camera_controller/arena_camera_input.h>
#include <render/camera/arena_camera.h>

#include <entt/entt.hpp>

namespace sanctify::pve {

/**
 * ECS structs around camera updating
 */
struct CameraUpdateState {
  ArenaCameraInputState inputState;
  indigo::core::Vector<IArenaCameraInput::ArenaCameraEvent> unhandledEvents;
};
struct GameCamera {
  ArenaCamera arenaCamera;
  float movementSpeed;
  float fovy;
  float aspectRatio;
};

/**
 * Utilities to help out with updating the camera
 */
class CameraUtils {
 public:
  static ArenaCamera& get_game_camera(entt::registry& world);
  static void set_update_state(entt::registry& world,
                               ArenaCameraInputState state);
  static void push_events(
      entt::registry& world,
      const indigo::core::Vector<IArenaCameraInput::ArenaCameraEvent>& evts);

  static void update_camera(entt::registry& world, float dt,
                            float aspect_ratio);
};

}  // namespace sanctify::pve

#endif
