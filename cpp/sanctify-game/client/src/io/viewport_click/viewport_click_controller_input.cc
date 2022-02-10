#include <io/viewport_click/viewport_click_controller_input.h>
#include <util/camera_util.h>

using namespace sanctify;
using namespace indigo;
using namespace core;

Maybe<IViewportClickControllerInput::ClickSurfaceEvent>
IViewportClickControllerInput::get_last_click() {
  auto action = nav_action_;
  nav_action_ = empty_maybe{};
  return action;
}

void IViewportClickControllerInput::fire_click_event(ClickSurfaceEvent evt) {
  nav_action_ = evt;
}

Maybe<NavigateToMapLocation> ViewportClickInput::get_frame_action(
    float fovy, float aspect, glm::vec3 camera_direction,
    glm::vec3 camera_position, glm::vec3 camera_up) {
  auto maybe_click_evt = input_->get_last_click();

  if (maybe_click_evt.is_empty()) {
    return empty_maybe{};
  }

  auto click_evt = maybe_click_evt.move();

  glm::vec3 pick_ray_direction = camera_util::get_camera_pick_ray(
      fovy, aspect, camera_direction, camera_up, click_evt.xPercent,
      click_evt.yPercent);

  Maybe<glm::vec3> xz_point =
      camera_util::pick_to_xz_plane(0.f, camera_position, pick_ray_direction);

  if (xz_point.is_empty()) {
    return empty_maybe{};
  }

  return NavigateToMapLocation{glm::vec2{xz_point.get().x, xz_point.get().z}};
}

void ViewportClickInput::attach() { input_->attach(); }

void ViewportClickInput::detach() { input_->detach(); }
