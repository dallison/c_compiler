// RUN: -std=c++23
// EXPECT_EXIT: 0

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cuchar>
#include <functional>
#include <iterator>
#include <string>
#include <utility>

int main() {
  if (std::byteswap(std::uint32_t{0x01020304}) != std::uint32_t{0x04030201}) {
    return 1;
  }
  enum class Color : unsigned { red = 3 };
  if (std::to_underlying(Color::red) != 3u) {
    return 2;
  }
  int value = 7;
  if (std::forward_like<const int&>(value) != 7) {
    return 3;
  }
  if (std::not_fn([](int n) { return n > 0; })(1)) {
    return 4;
  }
  auto add_two = std::bind_front([](int left, int right) { return left + right; }, 2);
  if (add_two(3) != 5) {
    return 5;
  }
  int numbers[] = {1, 2, 3};
  if (std::ssize(numbers) != 3) {
    return 6;
  }
  if (std::lerp(0.0, 10.0, 0.5) != 5.0) {
    return 7;
  }
  int shifted[] = {1, 2, 3, 4};
  if (std::shift_left(shifted, shifted + 4, 1) != shifted + 3 || shifted[0] != 2 ||
      shifted[2] != 4) {
    return 8;
  }
  int right[] = {1, 2, 3, 4};
  if (std::shift_right(right, right + 4, 1) != right + 1 || right[1] != 1 ||
      right[3] != 3) {
    return 9;
  }
  std::string text = "hello";
  if (!text.starts_with("he") || !text.ends_with("lo") || !text.contains("ell")) {
    return 10;
  }
  int found[] = {1, 2, 3, 2};
  auto last = std::ranges::find_last(found, 2);
  if (last.begin() == last.end() || *last.begin() != 2 || last.begin() != found + 3) {
    return 11;
  }
  alignas(16) char block[32];
  if (std::memalignment(static_cast<void*>(block)) < 16) {
    return 12;
  }
  if (std::memalignment(nullptr) != 0) {
    return 13;
  }
  char8_t unit = 0;
  std::mbstate_t state = {};
  const char* letters = "A";
  if (std::mbrtoc8(&unit, letters, 1, &state) != 1 || unit != u8'A') {
    return 14;
  }
  char encoded[4];
  std::mbstate_t out = {};
  if (std::c8rtomb(encoded, u8'A', &out) != 1 || encoded[0] != 'A') {
    return 15;
  }
  const char acute[] = "\xc3\xa9";
  char8_t units[2] = {};
  std::mbstate_t utf8 = {};
  if (std::mbrtoc8(&units[0], acute, 2, &utf8) != 2 || units[0] != 0xc3) {
    return 16;
  }
  if (std::mbrtoc8(&units[1], nullptr, 0, &utf8) != static_cast<std::size_t>(-3) ||
      units[1] != 0xa9) {
    return 17;
  }
  char roundtrip[4] = {};
  std::mbstate_t rebuild = {};
  if (std::c8rtomb(roundtrip, units[0], &rebuild) != 0) {
    return 18;
  }
  if (std::c8rtomb(roundtrip, units[1], &rebuild) != 2 ||
      static_cast<unsigned char>(roundtrip[0]) != 0xc3 ||
      static_cast<unsigned char>(roundtrip[1]) != 0xa9) {
    return 19;
  }
  if (false) {
    std::unreachable();
  }
  return 0;
}
