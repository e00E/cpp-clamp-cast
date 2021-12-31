In c/c++ it is easy to accidentally invoke undefined behavior (UB) when converting a floating point number to an integer:

https://en.cppreference.com/w/cpp/language/implicit_conversion section "Floating–integral conversions":
> A prvalue of floating-point type can be converted to a prvalue of any integer type. The fractional part is truncated, that is, the fractional part is discarded. **If the value cannot fit into the destination type, the behavior is undefined** (even when the destination type is unsigned, modulo arithmetic does not apply).


This applies to innocuous looking code like

```c++
void foo(float f) {
    int i0 = f;
    int i1 = int(f);
    int i2 = static_cast<int>(f);
}
```

which does not even generate a warning.

`clamp-cast.hpp` is a c++17 header that makes it easy to safely perform such conversions by clamping the value into the bounds of the target integer type.

```c++
void foo(float f) {
    int i = clamp_cast<int>(f);
}
```

`test.cpp` contains tests and examples and can be compiled and run with `./compile-and-test.sh`.

---

It is a sad state of affairs that c++ makes it so easy to accidentally invoke UB without providing a usable alternative in the standard library. The only roughly equivalent standard library function is (std::lround)[https://en.cppreference.com/w/cpp/numeric/math/round] which has problems:

* It is not constexpr.
* It does not get inlined.
* It potentially raises floating point exceptions which no one wants to work with.
* It returns an implementation defined value when the input is out of bounds instead of clamping to the bounds. Combined with the floating point exception this makes it hard to handle the out of bounds case.
* It can only return long and long long. So even if the initial conversion succeeds we need to bound check again when converting to a smaller integer.
