#ifndef SANCTIFY_COMMON_USER_INPUT_EVENTS_H
#define SANCTIFY_COMMON_USER_INPUT_EVENTS_H

#include <glm/glm.hpp>

namespace sanctify::io {

struct MouseMoveEvent {
  bool isPrimaryMouseDown;
  bool isSecondaryMouseDown;

  glm::vec2 startPos;
  glm::vec2 endPos;
};

struct MouseDownEvent {
  bool isPrimary;
  glm::vec2 pos;
};

struct MouseUpEvent {
  bool isPrimary;
  glm::vec2 pos;
};

enum class KeyType {
  OpenMenuKey,
  CenterCameraKey,
};

struct KeyDownEvent {
  KeyType keyType;
};

struct KeyUpEvent {
  KeyType keyType;
  float timeHeld;
};

struct KeyHoldEvent {
  KeyType keyType;
};

}  // namespace sanctify::io

#endif
