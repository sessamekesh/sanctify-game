#ifndef _LIB_IGCORE_FASTMATH_H_
#define _LIB_IGCORE_FASTMATH_H_

#include <cstddef>
#include <type_traits>

/**
 * Indigo FastMath library
 *
 * Use when speed is more important than exact accuracy
 *
 * Namespaced in its own namespace ("fastmath") since a lot of places use the
 * indigo::core namespace, and common names like sin/cos/min/max can easily
 * collide with names defined here.
 */

#include <cmath>
#include <cstdint>

namespace {
constexpr uint32_t kPowersOfTwo[32] = {
    0x1,        0x2,        0x4,       0x8,       0x10,       0x20,
    0x40,       0x80,       0x100,     0x200,     0x400,      0x800,
    0x1000,     0x2000,     0x4000,    0x8000,    0x10000,    0x20000,
    0x40000,    0x80000,    0x100000,  0x200000,  0x400000,   0x800000,
    0x1000000,  0x2000000,  0x4000000, 0x8000000, 0x10000000, 0x20000000,
    0x40000000, 0x80000000,
};

// https://stackoverflow.com/questions/4415530/equivalents-to-msvcs-countof-in-other-compilers
template <typename T, size_t N>
size_t countof(T (&arr)[N]) {
  return std::extent<T[N]>::value;
}

}  // namespace

namespace indigo::core::fastmath {
float lerp(float a, float b, float t);

float sin(float angle);
float cos(float angle);
float tan(float angle);
uint32_t log_2(uint32_t in);

constexpr bool is_power_of_two(uint32_t n) {
  // n = 0 is not a power of two - essential corner case because this algorithm
  //  depends on looking at (n-1), and that breaks if underload happens
  //
  // For all other cases, power of 2 integers will have a binary form with a
  //  exactly one single 1, all other digits being 0s.
  // Examples:
  // 1: 0000 0001
  // 2: 0000 0010
  // 4: 0000 0100
  //
  // Subtracting 1 from any of those numbers will give a number where there are
  //  a series of right-most 1s
  // 0: 0000 0000
  // 1: 0000 0001
  // 3: 0000 0011
  // 7: 0000 0111
  //
  // Therefore, the result of a bitwise AND against all bits in a number n
  //  and (n - 1) will be 0 if and only if n is a power of two
  //
  // Example: 8 & 7 = 0000 1000 & 0000 0111 = 0000 0000
  // But 9 & 8 = 0000 1001 & 0000 1000 = 0000 1000
  return n && (!(n & (n - 1)));
}

// Find the largest power of two less than or equal to the input
constexpr uint32_t largest_power_of_two_lte(uint32_t n) {
  if (n == 0) return 0;
  if (n == 1) return 1;
  for (int i = 1; i < countof(::kPowersOfTwo); i++) {
    if (n < kPowersOfTwo[i]) {
      return kPowersOfTwo[i - 1];
    }
  }

  // Should never reach this case
  return kPowersOfTwo[31];
}
}  // namespace indigo::core::fastmath

#endif
