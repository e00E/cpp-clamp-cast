#ifndef CLAMP_CAST_HPP
#define CLAMP_CAST_HPP

#include <limits>

namespace clamp_cast {

namespace detail {

// constexpr std::exp2 equivalent
template <typename T> constexpr T exp2(int exp) noexcept {
  // Alternatively we could use std::bit_cast but explicit exponentation is
  // clearer.
  T result{1.0};
  for (int i{0}; i < exp; ++i) {
    result *= 2.0f;
  }
  return result;
}

template <typename T> constexpr int exponent_bits() noexcept {
  using limits = std::numeric_limits<T>;
  static_assert(limits::is_iec559);
  static_assert(limits::radix == 2);
  return limits::max_exponent - 1;
}

} // namespace detail

// constexpr std::isnan equivalent
template <typename T> constexpr bool is_nan(T t) noexcept {
  static_assert(std::numeric_limits<T>::is_iec559);
  // https://en.cppreference.com/w/cpp/numeric/math/isnan
  return t != t;
}

// The lowest From value that will not underflow when converted to To.
template <typename To, typename From>
constexpr From lower_bound_inclusive() noexcept {
  using to_limits = std::numeric_limits<To>;
  static_assert(to_limits::is_integer);
  static_assert(to_limits::radix == 2);

  if constexpr (to_limits::is_signed) {
    constexpr auto to_bits = to_limits::digits;
    if constexpr (detail::exponent_bits<From>() >= to_bits) {
      return -detail::exp2<From>(to_bits);
    } else {
      return std::numeric_limits<From>::lowest;
    }
  } else {
    return 0.0;
  }
}

// The lowest From value that will overflow when converted to To.
template <typename To, typename From>
constexpr From upper_bound_exclusive() noexcept {
  using to_limits = std::numeric_limits<To>;
  static_assert(to_limits::is_integer);
  static_assert(to_limits::radix == 2);
  constexpr auto to_bits = to_limits::digits;

  if constexpr (detail::exponent_bits<From>() >= to_bits) {
    return detail::exp2<From>(to_bits);
  } else {
    return std::numeric_limits<From>::infinity;
  }
}

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
  // Floating point numbers can represent a large range of powers of 2 exactly.
  // For example, even a 32 bit float can represent 2**64. In this common case
  // we can represent the minimum and maximum values of To exactly in From.
  // The lower bound is inclusive and the upper exclusive because for integers
  // the minimum is a power of 2 while the maximum is a power of 2 minus 1.
  // In the uncommon case that a bound is larger than what From can exactly
  // represent we know that To can store all finite values of From because it
  // is at least one power of 2 larger.

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

  if (is_nan(from)) {
    return 0;
  } else if (from < lower_bound_inclusive<To, From>()) {
    return std::numeric_limits<To>::min();
  } else if (from >= upper_bound_exclusive<To, From>()) {
    return std::numeric_limits<To>::max();
  } else {
    return static_cast<To>(from);
  }
}

} // namespace clamp_cast

#endif
