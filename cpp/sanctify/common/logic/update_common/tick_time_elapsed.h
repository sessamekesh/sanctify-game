#ifndef SANCTIFY_COMMON_LOGIC_UPDATE_COMMON_TICK_TIME_ELAPSED_H
#define SANCTIFY_COMMON_LOGIC_UPDATE_COMMON_TICK_TIME_ELAPSED_H

#include <igecs/world_view.h>

namespace sanctify::logic {

struct CtxFrameTimeElapsed {
  float secondsElapsedSinceLastFrame;
};

struct CtxSimTime {
  float simTimeSeconds;

  bool operator==(const CtxSimTime& o) const {
    return simTimeSeconds == o.simTimeSeconds;
  }
};

class FrameTimeElapsedUtil {
 public:
  /** Update time elapsed and app wall time */
  static void mark_time_elapsed(indigo::igecs::WorldView* wv, float dt,
                                bool advance_sim = true);

  /**
   * How much time has elapsed since the last frame? (what was
   * "mark_time_elapsed" last called with)
   */
  static float dt(indigo::igecs::WorldView* wv);

  /** Hard set the sim time */
  static void set_sim_time(indigo::igecs::WorldView* wv, float sim_time);

  /** Read the sim time */
  static float get_sim_time(indigo::igecs::WorldView* wv);
};

}  // namespace sanctify::logic

#endif
