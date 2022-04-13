#include "io_system.h"

#include <pve_game_scene/ecs/camera.h>
#include <pve_game_scene/ecs/utils.h>

#include "glfw_io_system.h"

using namespace sanctify;
using namespace pve;
using namespace indigo;
using namespace core;

namespace {
const char* kLogLabel = "IoSystem";
}

void IoSystem::update_io(entt::registry& world) {
  auto glfw_input_state = GlfwIoSystem::get_arena_camera_input_state(world);
  if (glfw_input_state.has_value()) {
    CameraUtils::set_update_state(world, glfw_input_state.get());
  }

  CameraUtils::push_events(world, GlfwIoSystem::get_arena_camera_events(world));

  auto nav_evt = GlfwIoSystem::get_map_nav_event(world);
  if (nav_evt.has_value()) {
    Logger::log(kLogLabel) << "Creating nav event at location <"
                           << nav_evt.get().mapLocation.x << ", "
                           << nav_evt.get().mapLocation.y << ">";
    pb::GameClientSingleMessage msg{};

    pb::Vec2* travel_request =
        msg.mutable_travel_to_location_request()->mutable_destination();
    travel_request->set_x(nav_evt.get().mapLocation.x);
    travel_request->set_y(nav_evt.get().mapLocation.y);

    ecs::queue_client_message(world, msg);
  }
}
