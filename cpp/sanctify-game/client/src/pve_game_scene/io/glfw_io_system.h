#ifndef SANCTIFY_GAME_CLIENT_SRC_PVE_GAME_SCENE_IO_GLFW_IO_H
#define SANCTIFY_GAME_CLIENT_SRC_PVE_GAME_SCENE_IO_GLFW_IO_H

#include <app_base.h>
#include <igcore/maybe.h>
#include <io/arena_camera_controller/arena_camera_input.h>
#include <io/viewport_click/viewport_click_controller_input.h>

#include <entt/entt.hpp>

namespace sanctify::pve {

class GlfwIoSystem {
 public:
  // Attach everything this system does behind the scenes!
  static bool attach_glfw_io(entt::registry& world,
                             std::shared_ptr<AppBase> app_base);
  static void detach_glfw_io(entt::registry& world);

  // Arena camera controller...
  static indigo::core::Maybe<ArenaCameraInputState>
  get_arena_camera_input_state(entt::registry& world);
  static indigo::core::Vector<IArenaCameraInput::ArenaCameraEvent>
  get_arena_camera_events(entt::registry& world);

  // Viewport click controller...
  static indigo::core::Maybe<NavigateToMapLocation> get_map_nav_event(
      entt::registry& world);
};

}  // namespace sanctify::pve

#endif
