#ifndef SANCTIFY_GAME_CLIENT_SRC_IO_VIEWPORT_CLICK_VIEWPORT_CLICK_MOUSE_IMPL_H
#define SANCTIFY_GAME_CLIENT_SRC_IO_VIEWPORT_CLICK_VIEWPORT_CLICK_MOUSE_IMPL_H

#include <GLFW/glfw3.h>
#include <io/glfw_event_emitter.h>
#include <io/viewport_click/viewport_click_controller_input.h>

#include <memory>

namespace sanctify {

class ViewportClickMouseImpl
    : public IViewportClickControllerInput,
      public IMouseButtonListener,
      public IMousePositionListener,
      public std::enable_shared_from_this<ViewportClickMouseImpl> {
 public:
  using MouseButtonType = IMouseButtonListener::ButtonType;
  using MouseActionType = IMouseButtonListener::ActionType;

  static std::shared_ptr<ViewportClickMouseImpl> Create(GLFWwindow* window);

  // IMouseButtonListener
  void on_mouse_button(MouseButtonType button,
                       MouseActionType action_type) override;

  // IMousePositionListener
  void on_mouse_position_change(double x, double y) override;

  void attach() override;
  void detach() override;

 private:
  ViewportClickMouseImpl(GLFWwindow* window);

 private:
  GLFWwindow* window_;
  bool is_secondary_mouse_down_;
  bool is_primary_mouse_down_;

  double x_;
  double y_;
};

}  // namespace sanctify

#endif
