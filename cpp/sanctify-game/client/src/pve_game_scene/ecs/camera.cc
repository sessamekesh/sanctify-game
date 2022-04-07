#include "camera.h"

#include <pve_game_scene/ecs/client_config.h>

using namespace sanctify;
using namespace pve;

using namespace indigo;
using namespace core;

namespace {
GameCamera& get_game_camera(entt::registry& world) {
  auto* cam = world.try_ctx<GameCamera>();
  if (!cam) {
    auto& params = world.ctx_or_set<ClientConfigComponent>();
    world.set<GameCamera>(
        ArenaCamera(params.cameraDefaultLookAt, params.cameraDefaultTilt,
                    params.cameraDefaultSpin, params.cameraDefaultRadius),
        params.cameraDefaultMovementSpeed, params.cameraDefaultFovy, 1.f);
  }
  return world.ctx<GameCamera>();
}

ArenaCamera& arena_camera(entt::registry& world) {
  return ::get_game_camera(world).arenaCamera;
}

float movement_speed(entt::registry& world) {
  return ::get_game_camera(world).movementSpeed;
}

}  // namespace

ArenaCamera& CameraUtils::get_game_camera(entt::registry& world) {
  return ::arena_camera(world);
}

void CameraUtils::set_update_state(entt::registry& world,
                                   ArenaCameraInputState state) {
  world.ctx_or_set<CameraUpdateState>().inputState = state;
}

void CameraUtils::push_events(
    entt::registry& world,
    const Vector<IArenaCameraInput::ArenaCameraEvent>& evts) {
  world.ctx_or_set<CameraUpdateState>().unhandledEvents.append(evts);
}

void CameraUtils::update_camera(entt::registry& world, float dt,
                                float aspect_ratio) {
  ::get_game_camera(world).aspectRatio = aspect_ratio;

  auto& update_state = world.ctx_or_set<CameraUpdateState>();
  Vector<IArenaCameraInput::ArenaCameraEvent> events =
      std::move(update_state.unhandledEvents);
  auto& input_state = update_state.inputState;
  auto& camera = ::arena_camera(world);

  // Move camera according to x/y movement
  if (input_state.ScreenRightMovement != 0.f || input_state.ScreenUpMovement) {
    glm::vec3 camera_movement =
        (camera.screen_right() * input_state.ScreenRightMovement +
         camera.screen_up() * input_state.ScreenUpMovement) *
        ::movement_speed(world);
    camera.set_look_at(camera.look_at() + camera_movement);
  }

  // TODO (sessamekesh): Handle camera drag events here
}
