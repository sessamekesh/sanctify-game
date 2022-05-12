#ifndef SANCTIFY_COMMON_USER_INPUT_ECS_UTIL_H
#define SANCTIFY_COMMON_USER_INPUT_ECS_UTIL_H

#include <igcore/maybe.h>
#include <igcore/vector.h>
#include <igecs/world_view.h>

#include <variant>

#include "events.h"

namespace sanctify::io {

struct Event {
  typedef std::variant<MouseMoveEvent, MouseDownEvent, MouseUpEvent,
                       KeyDownEvent, KeyUpEvent, KeyHoldEvent>
      evt_type;

  evt_type evt;

  template <typename T>
  indigo::core::Maybe<T> get() const {
    if (std::holds_alternative<T>(evt)) {
      return std::get<T>(evt);
    }
    return indigo::core::empty_maybe{};
  }
};

class EcsUtil {
 public:
  // Events
  static void add_event(indigo::igecs::WorldView* wv, Event evt);
  static const indigo::igecs::WorldView::Decl& events_decl();
  static indigo::core::Vector<Event> get_events(indigo::igecs::WorldView* wv);

  // State
  static void set_mouse_pos(indigo::igecs::WorldView* wv, glm::vec2 pos);
  static void clear_mouse_pos(indigo::igecs::WorldView* wv);
  static indigo::core::Maybe<glm::vec2> get_mouse_pos(
      indigo::igecs::WorldView* wv);
};

}  // namespace sanctify::io

#endif
