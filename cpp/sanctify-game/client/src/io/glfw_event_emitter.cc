#include <igcore/log.h>
#include <io/glfw_event_emitter.h>

using namespace sanctify;

using namespace indigo;
using namespace core;

namespace {
const char* kLogLabel = "GlfwEventEmitter";

GLFWwindow* gWindow = nullptr;
std::shared_ptr<GlfwEventEmitter> gGlfwEventEmitter = nullptr;

std::shared_ptr<GlfwEventEmitter> get_global_emitter(GLFWwindow* window) {
  if (window == gWindow) {
    return gGlfwEventEmitter;
  }

  return nullptr;
}

bool button_type(int glfw_button, IMouseButtonListener::ButtonType& o_btn) {
  switch (glfw_button) {
    case GLFW_MOUSE_BUTTON_LEFT:
      o_btn = IMouseButtonListener::ButtonType::Left;
      return true;
    case GLFW_MOUSE_BUTTON_RIGHT:
      o_btn = IMouseButtonListener::ButtonType::Right;
      return true;
    case GLFW_MOUSE_BUTTON_MIDDLE:
      o_btn = IMouseButtonListener::ButtonType::Middle;
      return true;
    case GLFW_MOUSE_BUTTON_4:
      o_btn = IMouseButtonListener::ButtonType::Button1;
      return true;
    case GLFW_MOUSE_BUTTON_5:
      o_btn = IMouseButtonListener::ButtonType::Button2;
      return true;
    case GLFW_MOUSE_BUTTON_6:
      o_btn = IMouseButtonListener::ButtonType::Button3;
      return true;
    case GLFW_MOUSE_BUTTON_7:
      o_btn = IMouseButtonListener::ButtonType::Button4;
      return true;
  }

  return false;
}

bool action_type(int glfw_action, IMouseButtonListener::ActionType& o_action) {
  if (glfw_action == GLFW_PRESS) {
    o_action = IMouseButtonListener::ActionType::Press;
    return true;
  } else if (glfw_action == GLFW_RELEASE) {
    o_action = IMouseButtonListener::ActionType::Release;
    return true;
  }

  return false;
}

void glfw_mouse_listener(GLFWwindow* window, int glfw_button, int glfw_action,
                         int mods) {
  auto emitter = ::get_global_emitter(window);

  if (emitter == nullptr) {
    Logger::log(kLogLabel)
        << "Skipping mouse event - could not find emitter for window "
        << window;
    return;
  }

  IMouseButtonListener::ButtonType button{};
  if (!::button_type(glfw_button, button)) {
    Logger::log(kLogLabel)
        << "Skipping mouse event - unrecognized GLFW button type "
        << glfw_button;
    return;
  }

  IMouseButtonListener::ActionType action{};
  if (!::action_type(glfw_action, action)) {
    Logger::log(kLogLabel)
        << "Skipping mouse event - unrecognized GLFW action type "
        << glfw_action;
    return;
  }

  emitter->fire_mouse_event(button, action);
}

void glfw_mouse_pos_listener(GLFWwindow* window, double xpos, double ypos) {
  auto emitter = ::get_global_emitter(window);
  if (emitter == nullptr) {
    Logger::log(kLogLabel)
        << "Skipping mouse position event - could not find emitter for window "
        << window;
    return;
  }

  emitter->fire_mouse_position(xpos, ypos);
}

}  // namespace

std::shared_ptr<GlfwEventEmitter> GlfwEventEmitter::GetSingleton(
    GLFWwindow* window) {
  if (gGlfwEventEmitter == nullptr || gWindow != window) {
    auto emitter =
        std::shared_ptr<GlfwEventEmitter>(new GlfwEventEmitter(window));
    gWindow = window;
    gGlfwEventEmitter = emitter;

    glfwSetMouseButtonCallback(gWindow, ::glfw_mouse_listener);
    glfwSetCursorPosCallback(gWindow, ::glfw_mouse_pos_listener);
  }

  return gGlfwEventEmitter;
}

void GlfwEventEmitter::add_mouse_button_listener(
    const std::shared_ptr<IMouseButtonListener>& listener) {
  mouse_button_listeners_.erase(listener, false);
  mouse_button_listeners_.push_back(listener);
}

void GlfwEventEmitter::remove_mouse_button_listener(
    const std::shared_ptr<IMouseButtonListener>& listener) {
  mouse_button_listeners_.erase(listener, false);
}

void GlfwEventEmitter::fire_mouse_event(
    IMouseButtonListener::ButtonType button,
    IMouseButtonListener::ActionType action_type) {
  for (int i = 0; i < mouse_button_listeners_.size(); i++) {
    mouse_button_listeners_[i]->on_mouse_button(button, action_type);
  }
}

void GlfwEventEmitter::add_mouse_position_listener(
    const std::shared_ptr<IMousePositionListener>& listener) {
  mouse_position_listeners_.erase(listener, false);
  mouse_position_listeners_.push_back(listener);
}

void GlfwEventEmitter::remove_mouse_position_listener(
    const std::shared_ptr<IMousePositionListener>& listener) {
  mouse_position_listeners_.erase(listener, false);
}

void GlfwEventEmitter::fire_mouse_position(double x, double y) {
  for (int i = 0; i < mouse_position_listeners_.size(); i++) {
    mouse_position_listeners_[i]->on_mouse_position_change(x, y);
  }
}

GlfwEventEmitter::GlfwEventEmitter(GLFWwindow* window)
    : window_(window), mouse_button_listeners_(2) {}