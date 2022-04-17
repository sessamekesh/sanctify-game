#include "tick_time_elapsed.h"

using namespace sanctify;
using namespace logic;
using namespace indigo;

void FrameTimeElapsedUtil::mark_time_elapsed(igecs::WorldView* wv, float dt,
                                             bool advance_sim) {
  auto& component = wv->mut_ctx_or_set<CtxFrameTimeElapsed>(0.f);
  component.secondsElapsedSinceLastFrame = dt;

  if (advance_sim) {
    wv->mut_ctx_or_set<CtxSimTime>(0.f).simTimeSeconds += dt;
  }
}

float FrameTimeElapsedUtil::dt(igecs::WorldView* wv) {
  return wv->ctx<CtxFrameTimeElapsed>().secondsElapsedSinceLastFrame;
}

void FrameTimeElapsedUtil::set_sim_time(igecs::WorldView* wv, float sim_time) {
  wv->mut_ctx_or_set<CtxSimTime>().simTimeSeconds = sim_time;
}

float FrameTimeElapsedUtil::get_sim_time(igecs::WorldView* wv) {
  return wv->ctx<CtxSimTime>().simTimeSeconds;
}
