#include "ecs_util.h"

#include "glfw/desktop_event_emitter.h"

using namespace sanctify::io;

using namespace indigo;
using namespace core;

namespace {
struct CtxEventsQueue {
  indigo::core::Vector<Event> events;

  indigo::core::Maybe<glm::vec2> mousePos;
};
}  // namespace

void EcsUtil::add_event(indigo::igecs::WorldView* wv, Event evt) {
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

void EcsUtil::set_mouse_pos(igecs::WorldView* wv, glm::vec2 pos) {
  wv->mut_ctx_or_set<::CtxEventsQueue>().mousePos = pos;
}

void EcsUtil::clear_mouse_pos(igecs::WorldView* wv) {
  wv->mut_ctx_or_set<::CtxEventsQueue>().mousePos = empty_maybe{};
}

Maybe<glm::vec2> EcsUtil::get_mouse_pos(igecs::WorldView* wv) {
  return wv->mut_ctx_or_set<::CtxEventsQueue>().mousePos;
}
