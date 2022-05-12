#include "process_user_input.h"

#include <common/logic/update_common/tick_time_elapsed.h>
#include <common/render/common/render_components.h>
#include <common/render/viewport/update_arena_camera_system.h>
#include <common/user_input/ecs_util.h>
#include <sanctify/pve/net_logic_common/proto/client_messages.pb.h>

using namespace sanctify;
using namespace pve;
using namespace indigo;
using namespace core;

const igecs::WorldView::Decl& ProcessUserInputSystem::decl() {
  static const igecs::WorldView::Decl decl =
      igecs::WorldView::Decl()
          .evt_writes<sanctify::pve::proto::PveClientSingleMessage>()
          .ctx_reads<render::CtxPlatformObjects>()
          .ctx_reads<logic::CtxFrameTimeElapsed>()
          .ctx_writes<render::CtxArenaCameraInputs>()
          .ctx_reads<render::CtxArenaCamera>()
          .merge_in_decl(io::EcsUtil::events_decl());

  return decl;
}

void ProcessUserInputSystem::update(indigo::igecs::WorldView* wv) {
  auto& ctx_camera_inputs = wv->mut_ctx<render::CtxArenaCameraInputs>();
  const auto& ctx_render_components = wv->ctx<render::CtxPlatformObjects>();
  const auto& ctx_camera = wv->ctx<render::CtxArenaCamera>();
  const auto& dt = logic::FrameTimeElapsedUtil::dt(wv);

  ctx_camera_inputs.lookAtAdjustment = glm::vec3(0.f, 0.f, 0.f);

  // Pan the camera if the mouse is near the edge of the screen...
  Maybe<glm::vec2> mouse_pos = io::EcsUtil::get_mouse_pos(wv);
  if (mouse_pos.has_value()) {
    float xpct = mouse_pos.get().x / ctx_render_components.viewportWidth;
    float ypct = mouse_pos.get().y / ctx_render_components.viewportHeight;

    // TODO (sessamekesh): Pass in configuration state...
    const float mvt_speed = 10.f;
    const float screen_threshold_x = 0.2f;
    const float screen_threshold_y = 0.2f;

    // TODO (sessamekesh): Adjust these formulas to be correct!
    if (xpct < screen_threshold_x) {
      float scroll_strength = glm::sqrt(1.f - (xpct / screen_threshold_x));
      ctx_camera_inputs.lookAtAdjustment -=
          ctx_camera.arenaCamera.screen_right() * mvt_speed * dt *
          scroll_strength;
    } else if (xpct > (1.f - screen_threshold_x)) {
      float scroll_strength =
          glm::sqrt((xpct - (1.f - screen_threshold_x)) / screen_threshold_x);
      ctx_camera_inputs.lookAtAdjustment +=
          ctx_camera.arenaCamera.screen_right() * mvt_speed * dt *
          scroll_strength;
    }

    if (ypct < screen_threshold_y) {
      float scroll_strength = glm::sqrt(1.f - (ypct / screen_threshold_y));
      ctx_camera_inputs.lookAtAdjustment +=
          ctx_camera.arenaCamera.screen_up() * mvt_speed * dt * scroll_strength;
    } else if (ypct > (1.f - screen_threshold_y)) {
      float scroll_strength =
          glm::sqrt((ypct - (1.f - screen_threshold_y)) / screen_threshold_y);
      ctx_camera_inputs.lookAtAdjustment -=
          ctx_camera.arenaCamera.screen_up() * mvt_speed * dt * scroll_strength;
    }
  }

  // Handle events...
  Vector<io::Event> events = io::EcsUtil::get_events(wv);

  for (int i = 0; i < events.size(); i++) {
    const io::Event& evt = events[i];

    auto mouse_down_evt = evt.get<io::MouseDownEvent>();
    if (mouse_down_evt.has_value() && !mouse_down_evt.get().isPrimary) {
      // TODO (sessamekesh): Raycast this to get the map position!
      proto::PveClientSingleMessage msg{};
      auto* pb_pos = msg.mutable_player_movement_request()->mutable_position();
      pb_pos->set_x(0.2f);
      pb_pos->set_y(0.5f);
      wv->enqueue_event(std::move(msg));
    }
  }
}
