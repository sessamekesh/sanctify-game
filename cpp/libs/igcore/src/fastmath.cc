#include <igcore/fastmath.h>

#include <cmath>

using namespace indigo;
using namespace core;

namespace {
// TODO (sessamekesh): Put in a fast lookup table here

}  // namespace

float fastmath::lerp(float a, float b, float t) {
  return a * t + b * (1.f - t);
}

float fastmath::sin(float angle) {
  // TODO (sessamekesh): use a lookup table instead
  return sinf(angle);
}

float fastmath::cos(float angle) {
  // TODO (sessamekesh): use a lookup table instead
  return cosf(angle);
}

float fastmath::tan(float angle) {
  // TODO (sessamekesh): use a lookup table instead
  return tanf(angle);
}

uint32_t fastmath::log_2(uint32_t in) {
  // TODO (sessamekesh): Use an early-out lookup table instead
  return log2(in);
}