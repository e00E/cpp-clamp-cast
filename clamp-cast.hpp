#pragma once

#include <limits>

namespace details {

// std::isnan isn't constexpr according to cppreference so we implement our own.
template <typename T> constexpr bool isnan(T t) noexcept {
  // https://en.cppreference.com/w/cpp/numeric/math/isnan
  return t != t;
}

// std::exp2 and std::pow aren't constexpr.according to cppreference so we
// implement our own.
template <typename T> constexpr T exp2(int exp) noexcept {
  // Alternatively we could use std::bit_cast but explicit exponentation is
  // clearer.
  T result = 1.0;
  for (int i{0}; i < exp; ++i) {
    result *= 2.0;
  }
  return result;
}

} // namespace details

// Safe cast from a floating point type to an integer type by clamping to its
// bounds if the value would be outside.
// NaN is converted to 0.
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
  constexpr auto exponent_bits = from_limits::max_exponent - 1;

  // Floating point numbers can represent a large range of powers of 2 exactly.
  // For example, even a 32 bit float can represent 2**64. In this common case
  // we can represent the minimum and maximum values of To exactly in From.
  // The lower bound is inclusive and the upper exclusive because for integers
  // the minimum is a power of 2 while the maximum is a power of 2 minus 1.
  // In the uncommon case that a bound is larger than what From can exactly
  // represent we know that To can store all finite values of From because it
  // is at least one power of 2 larger.

  constexpr From lower_bound_inclusive = [&]() constexpr {
    if constexpr (to_limits::is_signed) {
      if constexpr (exponent_bits >= to_bits) {
        return -details::exp2<From>(to_bits);
      } else {
        return from_limits::lowest;
      }
    } else {
      return 0.0;
    }
  }
  ();

  constexpr From upper_bound_exclusive = [&]() constexpr {
    if constexpr (exponent_bits >= to_bits) {
      return details::exp2<From>(to_bits);
    } else {
      return from_limits::infinity;
    }
  }
  ();

  // Checking with the Godbolt compiler explorer and clang 10 we see that this
  // compiles to multiple jump instructions. It could be better to use
  // conditional moves but we were unable to get the compiler to emit them. The
  // assembly we would like is the the equivalent cast in Rust `pub fn f(f: f32)
  // -> i32 { f as i32 }`.
  // One problem in making this happen that unlike the Rust assembly we cannot
  // unconditionally start with `To to = static_cast<To>(from)` and then
  // conditionally overwrite if the limits are violated because the first
  // statement is already UB in C++.
  // For bounds that are powers of 2 we could use std::min, std::max to remove
  // the branching but this doesn't work for upper bounds as they are a power of
  // 2 minus 1 which is likely not exactly representable in From.

  if (details::isnan(from)) {
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
