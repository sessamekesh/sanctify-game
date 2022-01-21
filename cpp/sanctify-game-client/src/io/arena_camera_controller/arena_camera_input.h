#ifndef SANCTIFY_GAME_CLIENT_SRC_IO_ARENA_CAMERA_CONTROLLER_ARENA_CAMERA_INPUT_H
#define SANCTIFY_GAME_CLIENT_SRC_IO_ARENA_CAMERA_CONTROLLER_ARENA_CAMERA_INPUT_H

#include <igcore/vector.h>

#include <glm/glm.hpp>
#include <variant>

namespace sanctify {

struct ArenaCameraInputState {
  // What is the strength of upward movement by the client?
  float ScreenUpMovement;

  // What is the strength of rightward movement by the client?
  float ScreenRightMovement;
};

class IArenaCameraInput {
 public:
  struct SnapToPlayer {};

  struct DragCanvasPointToPoint {
    glm::vec2 ScreenSpaceStart;
    glm::vec2 ScreenSpaceFinish;
  };

  using ArenaCameraEvent = std::variant<SnapToPlayer, DragCanvasPointToPoint>;

 public:
  virtual void attach() {}
  virtual void detach() {}
  virtual void update(float dt) = 0;
  virtual indigo::core::Vector<ArenaCameraEvent> events_since_last_poll() = 0;
  virtual ArenaCameraInputState get_input_state() = 0;
};

}  // namespace sanctify

#endif
