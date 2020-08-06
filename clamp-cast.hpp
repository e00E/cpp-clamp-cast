#pragma once

#include <limits>

// std::isnan isn't constexpr according to cppreference
template <typename T> constexpr bool _isnan(T t) noexcept {
  static_assert(std::numeric_limits<T>::is_iec559);
  // https://en.cppreference.com/w/cpp/numeric/math/isnan
  return t != t;
}

// std::exp2 and std::pow aren't constexpr.according to cppreference
template <typename T> constexpr T _exp2(unsigned int exp) noexcept {
  static_assert(std::numeric_limits<T>::is_iec559);
  T result = 1.0;
  for (unsigned int i = 0; i < exp; ++i) {
    result *= 2.0;
  }
  return result;
}

// Safe cast from a floating point type to an integer type by clamping if the
// value would be out of bounds.
// Nan is converted to 0.
//
// Without clamping this would be undefined behavior if the value cannot fit
// into the destination type:
// https://en.cppreference.com/w/cpp/language/implicit_conversion
// section "Floating–integral conversions"
template <typename To, typename From>
constexpr To clamp_cast(const From from) noexcept {
  using from_limits = std::numeric_limits<From>;
  using to_limits = std::numeric_limits<To>;

  static_assert(from_limits::is_iec559);
  static_assert(to_limits::is_integer);
  static_assert(from_limits::radix == 2);
  static_assert(to_limits::radix == 2);

  constexpr auto to_bits = to_limits::digits;
  constexpr auto from_negative_exponent_bits = from_limits::min_exponent - 1;
  constexpr auto from_positive_exponent_bits = from_limits::max_exponent - 1;

  // Floating point numbers can represent a large range of powers of 2 exactly.
  // For example, even a 32 bit float can represent 2**64. In this common case
  // we can represent the minimum and maximum values of To exactly in From.
  // The lower bound is inclusive and the upper exclusive because for integers
  // the minimum is a power of 2 while the maximum is a power of 2 minus 1.
  // In the uncommon case that a bound is larger than what From can exactly
  // represent we know that To can store all finite values of From because it
  // is at least one more power of two larger.

  constexpr From lower_bound_inclusive = [&]() constexpr {
    if constexpr (to_limits::is_signed) {
      if constexpr (from_negative_exponent_bits <= -to_bits) {
        return -_exp2<From>(to_bits);
      } else {
        return from_limits::lowest;
      }
    } else {
      return 0.0;
    }
  }
  ();

  constexpr From upper_bound_exclusive = [&]() constexpr {
    if constexpr (from_positive_exponent_bits >= to_bits) {
      return _exp2<From>(to_bits);
    } else {
      return from_limits::infinity;
    }
  }
  ();

  if (_isnan(from)) {
    return 0;
  } else if (from < lower_bound_inclusive) {
    return to_limits::min();
  } else if (from >= upper_bound_exclusive) {
    return to_limits::max();
  } else {
    // We know that the value is not out of bounds for To. This cast is safe.
    return static_cast<To>(from);
  }
}
