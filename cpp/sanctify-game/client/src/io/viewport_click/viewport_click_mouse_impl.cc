#include <io/viewport_click/viewport_click_controller_input.h>
#include <io/viewport_click/viewport_click_mouse_impl.h>

#ifdef EMSCRIPTEN
#include <emscripten/html5.h>
#endif

using namespace sanctify;

using MouseButtonType = IMouseButtonListener::ButtonType;
using MouseActionType = IMouseButtonListener::ActionType;

std::shared_ptr<ViewportClickMouseImpl> ViewportClickMouseImpl::Create(
    GLFWwindow* window) {
  auto rsl = std::shared_ptr<ViewportClickMouseImpl>(
      new ViewportClickMouseImpl(window));
  return rsl;
}

ViewportClickMouseImpl::ViewportClickMouseImpl(GLFWwindow* window)
    : window_(window),
      is_secondary_mouse_down_(false),
      is_primary_mouse_down_(false),
      x_(0.),
      y_(0.) {}

void ViewportClickMouseImpl::on_mouse_button(MouseButtonType button_type,
                                             MouseActionType action_type) {
  if (button_type == MouseButtonType::Left) {
    is_primary_mouse_down_ = action_type == MouseActionType::Press;
  } else if (button_type == MouseButtonType::Right) {
    is_secondary_mouse_down_ = action_type == MouseActionType::Press;
  }

  if (button_type == MouseButtonType::Right &&
      action_type == MouseActionType::Release) {
    int width, height;
#ifdef EMSCRIPTEN
    emscripten_get_canvas_element_size("#app_canvas", &width, &height);
#else
    glfwGetWindowSize(window_, &width, &height);
#endif

    float xpct = x_ / width;
    float ypct = y_ / height;

    fire_click_event({xpct, ypct});
  }
}

void ViewportClickMouseImpl::on_mouse_position_change(double x, double y) {
  x_ = x;
  y_ = y;
}

void ViewportClickMouseImpl::attach() {
  auto self = shared_from_this();
  GlfwEventEmitter::GetSingleton(window_)->add_mouse_button_listener(self);
  GlfwEventEmitter::GetSingleton(window_)->add_mouse_position_listener(self);
}

void ViewportClickMouseImpl::detach() {
  auto self = shared_from_this();
  GlfwEventEmitter::GetSingleton(window_)->remove_mouse_button_listener(self);
  GlfwEventEmitter::GetSingleton(window_)->remove_mouse_position_listener(self);
}
