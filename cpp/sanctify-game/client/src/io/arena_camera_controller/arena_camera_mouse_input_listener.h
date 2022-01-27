#ifndef SANCTIFY_GAME_CLIENT_SRC_IO_ARENA_CAMERA_CONTROLLER_H
#define SANCTIFY_GAME_CLIENT_SRC_IO_ARENA_CAMERA_CONTROLLER_H

#include <GLFW/glfw3.h>
#include <io/arena_camera_controller/arena_camera_input.h>
#include <io/glfw_event_emitter.h>

namespace sanctify {

class ArenaCameraMouseInputListener
    : public IArenaCameraInput,
      public IMouseButtonListener,
      public IMousePositionListener,
      public std::enable_shared_from_this<ArenaCameraMouseInputListener> {
 public:
  using MouseButtonType = IMouseButtonListener::ButtonType;
  using MouseActionType = IMouseButtonListener::ActionType;

  static std::shared_ptr<ArenaCameraMouseInputListener> Create(
      GLFWwindow* window);

  // IArenaCameraInput
  void update(float dt) override {}
  indigo::core::Vector<ArenaCameraEvent> events_since_last_poll() override;
  ArenaCameraInputState get_input_state() override;
  void attach() override;
  void detach() override;

  // IMouseButtonListener
  void on_mouse_button(MouseButtonType button,
                       MouseActionType action_type) override;

  // IMousePositionListener
  void on_mouse_position_change(double x, double y) override;

 private:
  ArenaCameraMouseInputListener(GLFWwindow* window);

 private:
  GLFWwindow* window_;

  bool is_primary_mouse_down_;
  double last_x_;
  double last_y_;

  indigo::core::Vector<ArenaCameraEvent> pending_events_;
};

}  // namespace sanctify

#endif
