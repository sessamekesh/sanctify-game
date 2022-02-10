#ifndef SANCTIFY_GAME_CLIENT_SRC_IO_VIEWPORT_CLICK_VIEWPORT_CLICK_CONTROLLER_INPUT_H
#define SANCTIFY_GAME_CLIENT_SRC_IO_VIEWPORT_CLICK_VIEWPORT_CLICK_CONTROLLER_INPUT_H

#include <igcore/maybe.h>

#include <glm/glm.hpp>
#include <variant>

namespace sanctify {

class IViewportClickControllerInput {
 public:
  struct ClickSurfaceEvent {
    float xPercent;
    float yPercent;
  };

  indigo::core::Maybe<ClickSurfaceEvent> get_last_click();

 public:
  virtual void attach() {}
  virtual void detach() {}

  void fire_click_event(ClickSurfaceEvent event);

 protected:
  indigo::core::Maybe<ClickSurfaceEvent> nav_action_;
};

struct NavigateToMapLocation {
  glm::vec2 mapLocation;
};

class ViewportClickInput {
 public:
  ViewportClickInput(std::shared_ptr<IViewportClickControllerInput> input)
      : input_(input) {}

  indigo::core::Maybe<NavigateToMapLocation> get_frame_action(
      float fovy, float aspect, glm::vec3 camera_direction,
      glm::vec3 camera_position, glm::vec3 camera_up);

  void attach();
  void detach();

 private:
  std::shared_ptr<IViewportClickControllerInput> input_;
};

}  // namespace sanctify

#endif
