// RUN: -std=c++26
// EXPECT_EXIT: 0

#include <simd>
#include <array>
#include <complex>
#include <type_traits>
#include <version>

#if __cpp_lib_simd != 202411L
#error "incorrect __cpp_lib_simd"
#endif
#if __cpp_lib_simd_complex != 202502L
#error "incorrect __cpp_lib_simd_complex"
#endif

using ConstexprV = std::simd::vec<int, 4>;
constexpr ConstexprV constexpr_left(2);
constexpr ConstexprV constexpr_right(3);
constexpr ConstexprV constexpr_sum = constexpr_left + constexpr_right;
static_assert(constexpr_sum[0] == 5 && constexpr_sum[3] == 5);

int main() {
  using V = std::simd::vec<int, 4>;
  using M = V::mask_type;
  static_assert(V::size() == 4);
  static_assert(std::is_same<std::simd::resize_t<2, V>,
                             std::simd::vec<int, 2>>::value);
  static_assert(std::is_same<std::simd::rebind_t<float, V>,
                             std::simd::vec<float, 4>>::value);
  V a([](std::size_t i) { return static_cast<int>(i + 1); });
  V b(2);
  V c = a * b + V(1);
  if (c[0] != 3 || c[1] != 5 || c[2] != 7 || c[3] != 9)
    return 1;

  M selected = c > V(5);
  if (std::simd::reduce_count(selected) != 2 ||
      std::simd::reduce_min_index(selected) != 2 ||
      std::simd::reduce_max_index(selected) != 3)
    return 2;

  V chosen = std::simd::select(selected, c, V(0));
  if (chosen[0] != 0 || chosen[1] != 0 ||
      chosen[2] != 7 || chosen[3] != 9)
    return 3;
  if (std::simd::reduce(c) != 24 ||
      std::simd::reduce_min(c) != 3 ||
      std::simd::reduce_max(c) != 9)
    return 4;

  std::array<int, 3> short_input = {8, 6, 4};
  V loaded = std::simd::partial_load<V>(short_input);
  if (loaded[0] != 8 || loaded[1] != 6 ||
      loaded[2] != 4 || loaded[3] != 0)
    return 5;
  std::array<int, 4> output = {};
  std::simd::unchecked_store(loaded, output);
  if (output[0] != 8 || output[1] != 6 ||
      output[2] != 4 || output[3] != 0)
    return 6;

  auto reversed = std::simd::permute<4>(
      c, [](std::size_t i) { return 3 - i; });
  if (reversed[0] != 9 || reversed[3] != 3)
    return 7;
  V compressed = std::simd::compress(c, selected, -1);
  if (compressed[0] != 7 || compressed[1] != 9 ||
      compressed[2] != -1 || compressed[3] != -1)
    return 8;
  V expanded = std::simd::expand(compressed, selected, V(0));
  if (expanded[0] != 0 || expanded[1] != 0 ||
      expanded[2] != 7 || expanded[3] != 9)
    return 9;

  std::simd::vec<double, 2> squares(
      [](std::size_t i) { return i == 0 ? 9.0 : 16.0; });
  auto roots = std::sqrt(squares);
  if (roots[0] != 3.0 || roots[1] != 4.0)
    return 10;

  M bits(10u);
  if (bits.to_ullong() != 10u || bits[0] || !bits[1] ||
      bits[2] || !bits[3])
    return 11;

  V bit_values([](std::size_t i) { return 1 << i; });
  V shifted = (bit_values << V(1)) | V(1);
  if (shifted[0] != 3 || shifted[1] != 5 ||
      shifted[2] != 9 || shifted[3] != 17)
    return 12;
  int iterated_sum = 0;
  for (int value : c)
    iterated_sum += value;
  if (iterated_sum != 24)
    return 13;
  V sequence = std::simd::iota<V>;
  if (sequence[0] != 0 || sequence[1] != 1 ||
      sequence[2] != 2 || sequence[3] != 3)
    return 14;
  using ShortV = std::simd::vec<int, 2>;
  ShortV short_sum = ShortV(4) + ShortV(7);
  if (short_sum[0] != 11 || short_sum[1] != 11 ||
      !(short_sum > ShortV(10))[0])
    return 15;
  V negated = -a;
  V complemented = ~a;
  if (negated[0] != -1 || negated[3] != -4 ||
      complemented[0] != ~1 || complemented[3] != ~4)
    return 16;

  using NativeInt = std::simd::native_vec<int>;
  static_assert(NativeInt::size() ==
                std::simd::__native_vector_bytes / sizeof(int));
  NativeInt native_left(3);
  NativeInt native_right(5);
  NativeInt native_sum = native_left + native_right;
  if (native_sum[0] != 8 || native_sum[NativeInt::size() - 1] != 8)
    return 17;
  if (!(native_sum > NativeInt(7))[0])
    return 18;

  std::simd::native_vec<float> native_floats(
      [](std::size_t i) { return static_cast<float>(i + 1); });
  auto scaled = native_floats * std::simd::native_vec<float>(2.f) +
                std::simd::native_vec<float>(1.f);
  if (scaled[0] != 3.f || scaled[1] != 5.f)
    return 19;

  std::simd::vec<float, 4> packed_float(
      [](std::size_t i) { return static_cast<float>(i) + 0.5f; });
  auto packed_product = packed_float * std::simd::vec<float, 4>(2.f);
  if (packed_product[0] != 1.f || packed_product[3] != 7.f)
    return 20;
  if ((packed_product > std::simd::vec<float, 4>(4.f)).to_ullong() != 0xcu)
    return 21;

  using ComplexV = std::simd::vec<std::complex<float>, 2>;
  ComplexV left_c(std::complex<float>(1.f, 2.f));
  ComplexV right_c(std::complex<float>(3.f, 4.f));
  ComplexV sum_c = left_c + right_c;
  if (sum_c[0] != std::complex<float>(4.f, 6.f) ||
      sum_c[1] != std::complex<float>(4.f, 6.f))
    return 22;
  return 0;
}
