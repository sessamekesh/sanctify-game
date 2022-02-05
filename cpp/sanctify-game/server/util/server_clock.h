#ifndef SANCTIFY_GAME_SERVER_UTIL_SERVER_CLOCK_H
#define SANCTIFY_GAME_SERVER_UTIL_SERVER_CLOCK_H

#include <chrono>

namespace sanctify {

class ServerClock {
 public:
  static void reset();
  static float time();

 private:
  static std::chrono::high_resolution_clock::time_point start_t_;
};

}  // namespace sanctify

#endif
