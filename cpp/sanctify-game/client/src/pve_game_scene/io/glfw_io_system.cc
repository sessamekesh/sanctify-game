#include "glfw_io_system.h"

#include <GLFW/glfw3.h>
#include <igcore/log.h>
#include <io/arena_camera_controller/arena_camera_mouse_input_listener.h>
#include <io/glfw_event_emitter.h>
#include <io/viewport_click/viewport_click_mouse_impl.h>
#include <pve_game_scene/ecs/camera.h>

using namespace sanctify;
using namespace pve;
using namespace indigo;
using namespace core;

namespace {
const char* kLogLabel = "GlfwIoSystem";

struct CtxGlfwListenerEntity {
  entt::entity glfwEntity;
};

struct GlfwListenerComponent {
  GLFWwindow* window;
  std::shared_ptr<ArenaCameraMouseInputListener> arenaCameraMouseInputListener;
  std::shared_ptr<ViewportClickMouseImpl> viewportClickController;

  std::shared_ptr<ViewportClickInput> viewportClickInput;
};

void on_destroy_listener(entt::registry& world, entt::entity e) {
  Logger::log(kLogLabel) << "Destroyed PVE scene listeners";
  auto& c = world.get<GlfwListenerComponent>(e);
  c.arenaCameraMouseInputListener->detach();
  c.viewportClickController->detach();
}

}  // namespace

bool GlfwIoSystem::attach_glfw_io(entt::registry& world,
                                  std::shared_ptr<AppBase> app_base) {
  Logger::log(kLogLabel) << "Attaching GLFW io resources";
  detach_glfw_io(world);

  world.on_destroy<GlfwListenerComponent>().connect<&::on_destroy_listener>();

  auto e = world.create();
  auto arena_camera_mouse_input_listener =
      ArenaCameraMouseInputListener::Create(app_base->Window);
  auto viewport_click_controller =
      ViewportClickMouseImpl::Create(app_base->Window);

  if (!arena_camera_mouse_input_listener || !viewport_click_controller) {
    return false;
  }

  arena_camera_mouse_input_listener->attach();
  viewport_click_controller->attach();

  auto viewport_click_input =
      std::make_shared<ViewportClickInput>(viewport_click_controller);

  world.emplace<GlfwListenerComponent>(
      e, app_base->Window, arena_camera_mouse_input_listener,
      viewport_click_controller, viewport_click_input);
  world.set<CtxGlfwListenerEntity>(e);
}

void GlfwIoSystem::detach_glfw_io(entt::registry& world) {
  if (world.try_ctx<CtxGlfwListenerEntity>()) {
    world.destroy(world.ctx<CtxGlfwListenerEntity>().glfwEntity);
    world.unset<CtxGlfwListenerEntity>();
    world.on_destroy<GlfwListenerComponent>()
        .disconnect<&::on_destroy_listener>();
  }
}

Maybe<ArenaCameraInputState> GlfwIoSystem::get_arena_camera_input_state(
    entt::registry& world) {
  auto* ctx = world.try_ctx<CtxGlfwListenerEntity>();
  if (!ctx) return empty_maybe{};

  auto* listeners = world.try_get<GlfwListenerComponent>(ctx->glfwEntity);
  if (!listeners) return empty_maybe{};

  return listeners->arenaCameraMouseInputListener->get_input_state();
}

Vector<IArenaCameraInput::ArenaCameraEvent>
GlfwIoSystem::get_arena_camera_events(entt::registry& world) {
  auto* ctx = world.try_ctx<CtxGlfwListenerEntity>();
  if (!ctx) return {};

  auto* listeners = world.try_get<GlfwListenerComponent>(ctx->glfwEntity);
  if (!listeners) return {};

  return listeners->arenaCameraMouseInputListener->events_since_last_poll();
}

Maybe<NavigateToMapLocation> GlfwIoSystem::get_map_nav_event(
    entt::registry& world) {
  auto* ctx = world.try_ctx<CtxGlfwListenerEntity>();
  if (!ctx) return empty_maybe{};

  auto* listeners = world.try_get<GlfwListenerComponent>(ctx->glfwEntity);
  if (!listeners) return empty_maybe{};

  auto* camera = world.try_ctx<GameCamera>();
  if (!camera) return empty_maybe{};

  // Require camera parameters here...
  return listeners->viewportClickInput->get_frame_action(
      camera->fovy, camera->aspectRatio,
      camera->arenaCamera.look_at() - camera->arenaCamera.position(),
      camera->arenaCamera.position(), camera->arenaCamera.screen_up());
}
