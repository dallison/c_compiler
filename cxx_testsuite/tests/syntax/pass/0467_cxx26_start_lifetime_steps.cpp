// RUN: -std=c++26

#include <memory>

struct element {
  int number;
};

union storage {
  element elements[2];
  int fallback;
};

constexpr bool default_only() {
  storage value;
  return true;
}

constexpr bool start_only() {
  storage value;
  std::start_lifetime(value.elements);
  return true;
}

constexpr int construct_one() {
  storage value;
  std::start_lifetime(value.elements);
  std::construct_at(&value.elements[1], element{42});
  return value.elements[1].number;
}

static_assert(default_only());
static_assert(start_only());
static_assert(construct_one() == 42);
