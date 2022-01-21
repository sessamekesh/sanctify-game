#include <io/arena_camera_controller/arena_camera_mouse_input_listener.h>
#include <io/glfw_event_emitter.h>

using namespace sanctify;

std::shared_ptr<ArenaCameraMouseInputListener>
ArenaCameraMouseInputListener::Create(GLFWwindow* window) {
  auto rsl = std::shared_ptr<ArenaCameraMouseInputListener>(
      new ArenaCameraMouseInputListener(window));
  return rsl;
}

void ArenaCameraMouseInputListener::attach() {
  auto self = shared_from_this();
  GlfwEventEmitter::GetSingleton(window_)->add_mouse_button_listener(self);
  GlfwEventEmitter::GetSingleton(window_)->add_mouse_position_listener(self);
}

void ArenaCameraMouseInputListener::detach() {
  auto self = shared_from_this();
  GlfwEventEmitter::GetSingleton(window_)->remove_mouse_button_listener(self);
  GlfwEventEmitter::GetSingleton(window_)->remove_mouse_position_listener(self);
}

ArenaCameraMouseInputListener::ArenaCameraMouseInputListener(GLFWwindow* window)
    : window_(window),
      is_primary_mouse_down_(false),
      last_x_(300.f),
      last_y_(300.f) {}

indigo::core::Vector<IArenaCameraInput::ArenaCameraEvent>
ArenaCameraMouseInputListener::events_since_last_poll() {
  indigo::core::Vector<IArenaCameraInput::ArenaCameraEvent> events =
      std::move(pending_events_);

  // TODO (sessamekesh): fix this whole file, mouse drag is crap
  pending_events_ =
      indigo::core::Vector<IArenaCameraInput::ArenaCameraEvent>(1);
  if (is_primary_mouse_down_) {
    if (std::holds_alternative<DragCanvasPointToPoint>(events.last())) {
      pending_events_.push_back(events.last());
      std::get<DragCanvasPointToPoint>(pending_events_.last())
          .ScreenSpaceStart =
          std::get<DragCanvasPointToPoint>(pending_events_.last())
              .ScreenSpaceFinish;
    }
  }

  return events;
}

ArenaCameraInputState ArenaCameraMouseInputListener::get_input_state() {
  int width, height;
  glfwGetWindowSize(window_, &width, &height);

  float screen_up_movement = 0.f;
  float screen_right_movement = 0.f;

  float fwidth = (float)width;
  float fheight = (float)height;

  if (last_x_ < (fwidth / 10.f)) {
    float pct = ((fwidth / 10.f) - last_x_) / (fwidth / 10.f);
    screen_right_movement = pct;
  } else if (last_x_ > (fwidth * 0.9f)) {
    float pct = (last_x_ - (fwidth * 0.9f)) / (fwidth * 0.1f);
    screen_right_movement = -pct;
  }

  if (last_y_ < (fheight / 10.f)) {
    float pct = ((fheight / 10.f) - last_y_) / (fheight / 10.f);
    screen_up_movement = pct;
  } else if (last_y_ > (fheight * 0.9f)) {
    float pct = (last_y_ - (fheight * 0.9f)) / (fheight * 0.1f);
    screen_up_movement = -pct;
  }

  return {screen_up_movement, screen_right_movement};
}

void ArenaCameraMouseInputListener::on_mouse_button(MouseButtonType button,
                                                    MouseActionType action) {
  if (button == MouseButtonType::Left) {
    is_primary_mouse_down_ = action == MouseActionType::Press;
  }
}

void ArenaCameraMouseInputListener::on_mouse_position_change(double x,
                                                             double y) {
  double dx = x - last_x_;
  double dy = y - last_y_;
  last_x_ = x;
  last_y_ = y;

  if (!is_primary_mouse_down_) {
    return;
  }

  if (pending_events_.size() > 0) {
    auto& last_event = pending_events_.last();
    if (std::holds_alternative<DragCanvasPointToPoint>(last_event)) {
      auto& drag_canvas_event = std::get<DragCanvasPointToPoint>(last_event);
      drag_canvas_event.ScreenSpaceFinish.x = x;
      drag_canvas_event.ScreenSpaceFinish.y = y;
      return;
    }
  }

  DragCanvasPointToPoint new_evt{};
  new_evt.ScreenSpaceStart.x = x;
  new_evt.ScreenSpaceStart.y = y;
  new_evt.ScreenSpaceFinish = new_evt.ScreenSpaceStart;
  pending_events_.push_back(new_evt);
}