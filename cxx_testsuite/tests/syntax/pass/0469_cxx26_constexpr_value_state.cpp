// RUN: -std=c++26 -fconstexpr-eval=audit

#include <cstddef>

constexpr int overwritten_scalar() {
  int value;
  value = 42;
  return value;
}

constexpr bool uninitialized_friendly_copy() {
  unsigned char source;
  unsigned char destination = source;
  destination = 7;
  return destination == 7;
}

constexpr bool byte_friendly_copy() {
  std::byte source;
  std::byte destination = source;
  destination = static_cast<std::byte>(7);
  return std::to_integer<int>(destination) == 7;
}

constexpr int unreachable_erroneous_read() {
  int value;
  if (false) {
    return value;
  }
  return 9;
}

static_assert(overwritten_scalar() == 42);
static_assert(uninitialized_friendly_copy());
static_assert(byte_friendly_copy());
static_assert(unreachable_erroneous_read() == 9);
