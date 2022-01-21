#ifndef SANCTIFY_GAME_CLIENT_SRC_IO_GLFW_EVENT_EMITTER_H
#define SANCTIFY_GAME_CLIENT_SRC_IO_GLFW_EVENT_EMITTER_H

#include <GLFW/glfw3.h>
#include <igcore/vector.h>

#include <memory>

namespace sanctify {

class IMouseButtonListener {
 public:
  enum class ButtonType {
    Left,
    Right,
    Middle,

    Button1,
    Button2,
    Button3,
    Button4,
  };

  enum class ActionType {
    Press,
    Release,
  };

 public:
  virtual void on_mouse_button(ButtonType button, ActionType action_type) = 0;
};

class IMousePositionListener {
 public:
  virtual void on_mouse_position_change(double x, double y) = 0;
};

class GlfwEventEmitter : std::enable_shared_from_this<GlfwEventEmitter> {
 public:
  static std::shared_ptr<GlfwEventEmitter> GetSingleton(GLFWwindow* window);

  // Mouse button presses
  void add_mouse_button_listener(
      const std::shared_ptr<IMouseButtonListener>& listener);
  void remove_mouse_button_listener(
      const std::shared_ptr<IMouseButtonListener>& listener);
  void fire_mouse_event(IMouseButtonListener::ButtonType button,
                        IMouseButtonListener::ActionType action_type);

  // Mouse movement
  void add_mouse_position_listener(
      const std::shared_ptr<IMousePositionListener>& listener);
  void remove_mouse_position_listener(
      const std::shared_ptr<IMousePositionListener>& listener);
  void fire_mouse_position(double x, double y);

 private:
  GlfwEventEmitter(GLFWwindow* window);

 private:
  GLFWwindow* window_;

  indigo::core::Vector<std::shared_ptr<IMouseButtonListener>>
      mouse_button_listeners_;
  indigo::core::Vector<std::shared_ptr<IMousePositionListener>>
      mouse_position_listeners_;
};

}  // namespace sanctify

#endif
