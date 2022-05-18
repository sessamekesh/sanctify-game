#ifndef SANCTIFY_COMMON_USER_INPUT_EVENTS_H
#define SANCTIFY_COMMON_USER_INPUT_EVENTS_H

#include <glm/glm.hpp>

namespace sanctify::io {

struct MouseMoveEvent {
  bool isPrimaryMouseDown;
  bool isSecondaryMouseDown;

  glm::vec2 startPos;
  glm::vec2 endPos;

  static MouseMoveEvent of(bool is_primary_down, bool is_secondary_down,
                           glm::vec2 start_pos, glm::vec2 end_pos) {
    return MouseMoveEvent{is_primary_down, is_secondary_down, start_pos,
                          end_pos};
  }
};

struct FocusChangeEvent {
  bool isFocused;

  static FocusChangeEvent of(bool is_focused) {
    return FocusChangeEvent{is_focused};
  }
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
