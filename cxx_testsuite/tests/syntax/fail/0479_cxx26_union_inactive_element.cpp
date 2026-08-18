// RUN: -std=c++26
// EXPECT: static_assert expression is not an integer constant expression

#include <memory>

union plain_storage {
  int first;
  int second;
};

constexpr int read_after_default_construction() {
  plain_storage value;
  return value.first;
}

struct element {
  int number;
};

union array_storage {
  element elements[2];
  int fallback;
};

constexpr int read_unstarted_array_element() {
  array_storage value;
  std::start_lifetime(value.elements);
  std::construct_at(&value.elements[1], element{42});
  return value.elements[0].number;
}

constexpr int read_destroyed_array_element() {
  array_storage value;
  std::start_lifetime(value.elements);
  std::construct_at(&value.elements[0], element{42});
  std::destroy_at(&value.elements[0]);
  return value.elements[0].number;
}

static_assert(read_after_default_construction() == 0);
static_assert(read_unstarted_array_element() == 0);
static_assert(read_destroyed_array_element() == 0);
