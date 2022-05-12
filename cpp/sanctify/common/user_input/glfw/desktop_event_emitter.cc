#include "desktop_event_emitter.h"

#include "../ecs_util.h"

using namespace sanctify;
using namespace io;
using namespace indigo;

namespace {

GlfwDesktopEventEmitter* gInstance = nullptr;

struct CtxMouseState {
  double x;
  double y;
};

void glfw_cursor_pos_callback(GLFWwindow* window, double xpos, double ypos) {
  if (!gInstance || !gInstance->world()) return;

  auto* wv = gInstance->thin_view();
  auto& ctx = wv->mut_ctx_or_set<CtxMouseState>(xpos, ypos);

  bool is_primary_down = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
  bool is_secondary_down = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);

  EcsUtil::add_event(wv, Event{MouseMoveEvent{
                             is_primary_down, is_secondary_down,
                             glm::vec2{ctx.x, ctx.y}, glm::vec2{xpos, ypos}}});
  EcsUtil::set_mouse_pos(wv, glm::vec2{xpos, ypos});

  ctx.x = xpos;
  ctx.y = ypos;
}

void glfw_cursor_enter_callback(GLFWwindow* window, int entered) {
  if (!gInstance || !gInstance->world()) return;
  if (!entered) {
    EcsUtil::clear_mouse_pos(gInstance->thin_view());
  }
}

void glfw_mouse_button_callback(GLFWwindow* window, int button, int action,
                                int mods) {
  if (!gInstance || !gInstance->world()) return;

  auto* wv = gInstance->thin_view();

  double xpos = 0., ypos = 0.;
  glfwGetCursorPos(window, &xpos, &ypos);

  bool is_primary = button == GLFW_MOUSE_BUTTON_LEFT;
  if (action == GLFW_PRESS) {
    EcsUtil::add_event(
        wv, Event{MouseDownEvent{is_primary, glm::vec2(xpos, ypos)}});
  } else if (action == GLFW_RELEASE) {
    EcsUtil::add_event(wv,
                       Event{MouseUpEvent{is_primary, glm::vec2(xpos, ypos)}});
  }
}

}  // namespace

GlfwDesktopEventEmitter::GlfwDesktopEventEmitter(entt::registry* world)
    : window_(nullptr),
      world_(world),
      thin_view_(igecs::WorldView::Thin(world)) {}

GlfwDesktopEventEmitter::~GlfwDesktopEventEmitter() {
  if (window_) {
    detach();
  }
}

void GlfwDesktopEventEmitter::attach_to_window(GLFWwindow* window) {
  if (window_) {
    detach();
  }

  assert(::gInstance == nullptr);
  ::gInstance = this;

  window_ = window;

  glfwSetCursorPosCallback(window_, ::glfw_cursor_pos_callback);
  glfwSetMouseButtonCallback(window_, ::glfw_mouse_button_callback);
  glfwSetCursorEnterCallback(window_, ::glfw_cursor_enter_callback);
}

void GlfwDesktopEventEmitter::detach() {
  glfwSetCursorEnterCallback(window_, nullptr);
  glfwSetMouseButtonCallback(window_, nullptr);
  glfwSetCursorPosCallback(window_, nullptr);

  ::gInstance = nullptr;
}
