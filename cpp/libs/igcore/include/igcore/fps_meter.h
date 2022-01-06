#ifndef LIBS_IGCORE_FPS_METER_H
#define LIBS_IGCORE_FPS_METER_H

/**
 * Basic FPS meter - measure how long has elapsed for a frame, and report on it
 *  every "x" seconds.
 */

#include <cstdint>
#include <string>

namespace indigo {
namespace core {

/** Console reporter - performs a std out message every x interval with FPS */
class ConsoleFpsMeter {
 public:
  ConsoleFpsMeter(float report_interval = 5.f,
                  std::string counter_name = "ConsoleFpsMeter",
                  float decay_rate = 0.9f);

  void tick(float dt);

 private:
  float dt_since_last_report_;
  uint32_t frame_count_since_last_report_;

  float max_frame_time_in_interval_;

  std::string name_;

  float time_between_reports_;

  float running_fps_;
  float decay_rate_;
};

}  // namespace core
}  // namespace indigo

#endif
