#include "ecs_util.h"

using namespace sanctify::io;

using namespace indigo;
using namespace core;

namespace {

struct CtxEventsQueue {
  indigo::core::Vector<Event> events;
  glm::vec2 lastMousePos;
  bool isFocused;
};
}  // namespace

void EcsUtil::add_event(indigo::igecs::WorldView* wv, Event evt) {
  evt.get<FocusChangeEvent>().if_present([wv](const io::FocusChangeEvent& evt) {
    wv->mut_ctx_or_set<::CtxEventsQueue>().isFocused = evt.isFocused;
  });
  evt.get<MouseMoveEvent>().if_present([wv](const io::MouseMoveEvent& evt) {
    wv->mut_ctx_or_set<::CtxEventsQueue>().lastMousePos = evt.endPos;
  });

  wv->mut_ctx_or_set<CtxEventsQueue>().events.push_back(evt);
}

const indigo::igecs::WorldView::Decl& EcsUtil::events_decl() {
  static indigo::igecs::WorldView::Decl decl =
      indigo::igecs::WorldView::Decl().ctx_writes<::CtxEventsQueue>();

  return decl;
}

indigo::core::Vector<Event> EcsUtil::get_events(indigo::igecs::WorldView* wv) {
  auto& ctx_queue = wv->mut_ctx_or_set<::CtxEventsQueue>();

  indigo::core::Vector<Event> evts = ctx_queue.events;
  ctx_queue.events.clear();

  return evts;
}

Maybe<glm::vec2> EcsUtil::get_mouse_pos(igecs::WorldView* wv) {
  auto& ctx_events_queue = wv->mut_ctx_or_set<::CtxEventsQueue>();
  if (!ctx_events_queue.isFocused) {
    return empty_maybe{};
  }
  return ctx_events_queue.lastMousePos;
}
