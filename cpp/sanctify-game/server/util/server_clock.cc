#include <util/server_clock.h>

using namespace sanctify;

namespace {
using FpSeconds = std::chrono::duration<float, std::chrono::seconds::period>;
}

std::chrono::high_resolution_clock::time_point ServerClock::start_t_ =
    std::chrono::high_resolution_clock::now();

void ServerClock::reset() {
  start_t_ = std::chrono::high_resolution_clock::now();
}

float ServerClock::time() {
  auto now = std::chrono::high_resolution_clock::now();

  return FpSeconds(now - start_t_).count();
}