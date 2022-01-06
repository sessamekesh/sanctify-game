#include <igcore/fps_meter.h>
#include <igcore/log.h>

#include <iomanip>

using namespace indigo::core;

ConsoleFpsMeter::ConsoleFpsMeter(float report_interval,
                                 std::string counter_name, float decay_rate)
    : dt_since_last_report_(0.f),
      frame_count_since_last_report_(0u),
      max_frame_time_in_interval_(-1.f),
      name_(counter_name),
      time_between_reports_(report_interval),
      running_fps_(-1.f),
      decay_rate_(decay_rate) {}

void ConsoleFpsMeter::tick(float dt) {
  dt_since_last_report_ += dt;
  frame_count_since_last_report_++;

  if (dt > max_frame_time_in_interval_) {
    max_frame_time_in_interval_ = dt;
  }

  if (dt_since_last_report_ > time_between_reports_ &&
      frame_count_since_last_report_ > 0) {
    float fps = frame_count_since_last_report_ / dt_since_last_report_;
    if (running_fps_ >= 0.f) {
      fps = running_fps_ * decay_rate_ + (1.0 - decay_rate_) * fps;
    }
    running_fps_ = fps;

    core::Logger::log("FPS - " + name_)
        << "\n  FPS: " << std::setprecision(3) << fps
        << "\n  Longest frame: " << max_frame_time_in_interval_ * 1000.f
        << "ms (" << 1.f / max_frame_time_in_interval_ << " fps)";

    max_frame_time_in_interval_ = -1.f;
    dt_since_last_report_ = 0.f;
    frame_count_since_last_report_ = 0u;
  }
}