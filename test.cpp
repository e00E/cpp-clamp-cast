#include <cfloat>
#include <cmath>
#include <cstdint>
#include <iostream>

#include "clamp-cast.hpp"

template <typename To, typename From> bool test_case(From from, To expected) {
  const auto result = clamp_cast<To, From>(from);
  if (result != expected) {
    // there is a `+` in front of expected to prevent cout from treating uint8
    // as char and not printing it as a decimal number.
    std::cout << "clamp_cast(" << from << ") == " << +result
              << " != " << +expected << "\n";
    return false;
  }
  return true;
}

bool test() {
  // works as constexpr
  static_assert(clamp_cast<uint8_t, float>(0.0) == 0);

  bool success = true;

  success &= test_case<uint8_t, float>(NAN, 0);
  success &= test_case<uint8_t, float>(0.0, 0);
  success &= test_case<uint8_t, float>(1.0, 1);
  success &= test_case<uint8_t, float>(-1.0, 0);
  success &= test_case<uint8_t, float>(254.0, 254);
  success &= test_case<uint8_t, float>(255.0, 255);
  success &= test_case<uint8_t, float>(256.0, 255);
  success &= test_case<uint8_t, float>(FLT_MAX, 255);
  success &= test_case<uint8_t, float>(-FLT_MAX, 0);

  success &= test_case<int8_t, float>(NAN, 0);
  success &= test_case<int8_t, float>(0.0, 0);
  success &= test_case<int8_t, float>(1.0, 1);
  success &= test_case<int8_t, float>(-1.0, -1);
  success &= test_case<int8_t, float>(126.0, 126);
  success &= test_case<int8_t, float>(127.0, 127);
  success &= test_case<int8_t, float>(128.0, 127);
  success &= test_case<int8_t, float>(-127.0, -127);
  success &= test_case<int8_t, float>(-128.0, -128);
  success &= test_case<int8_t, float>(-129.0, -128);
  success &= test_case<int8_t, float>(FLT_MAX, 127);
  success &= test_case<int8_t, float>(-FLT_MAX, -128);

  const float bound{std::exp2f(63.0)};
  // `- 1` because minimum int64_t cannot be written as integer literal
  // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=52661 .
  success &= test_case<int64_t, float>(std::nextafter(-bound, -INFINITY),
                                       -9223372036854775807ll - 1);
  success &= test_case<int64_t, float>(-bound, -9223372036854775807ll - 1);
  success &= test_case<int64_t, float>(std::nextafter(-bound, INFINITY),
                                       -9223371487098961920ll);

  success &= test_case<int64_t, float>(std::nextafter(bound, -INFINITY),
                                       9223371487098961920ll);
  success &= test_case<int64_t, float>(bound, 9223372036854775807ll);
  success &= test_case<int64_t, float>(std::nextafter(bound, INFINITY),
                                       9223372036854775807ll);

  // We can't test the uncommon case mentioned in the comment in clamp_cast
  // because we would need a uint128_t which does not exist. Some compilers have
  // a __uint128_t but it does not implement std::numeric_limits.

  return success;
}

int main() {
  if (test()) {
    std::cout << "no errors\n";
    return 0;
  } else {
    return 1;
  }
}
