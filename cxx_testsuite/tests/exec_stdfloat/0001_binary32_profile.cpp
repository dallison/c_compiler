// RUN: -std=c++23
// EXPECT_EXIT: 0

#include <limits>
#include <stdfloat>
#include <type_traits>

static_assert(sizeof(std::float32_t) == 4);
static_assert(!std::is_same_v<std::float32_t, float>);
static_assert(std::is_floating_point_v<std::float32_t>);
static_assert(std::numeric_limits<std::float32_t>::digits == 24);

std::float32_t input = 1.25f32;

std::float32_t add(std::float32_t left, std::float32_t right) {
  return left + right;
}

int main() {
  std::float32_t result = add(input, 0.75f32);
  return result == 2.0f32 ? 0 : 1;
}
