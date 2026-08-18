// RUN: -std=c++26

#include <memory>
#include <type_traits>
#include <version>

#if __cpp_lib_start_lifetime != 202603L
#error "__cpp_lib_start_lifetime must advertise the final C++26 wording"
#endif

struct value {
  int number;
};

union storage {
  value elements[4];
  int fallback;
};

struct duplicate_value {
  int number;
};

union duplicate_storage {
  duplicate_value first;
  duplicate_value second;
};

constexpr storage make_storage() {
  storage result;
  std::start_lifetime(result.elements);
  std::construct_at(&result.elements[1], value{20});
  std::construct_at(&result.elements[3], value{22});
  return result;
}

constexpr storage make_dirty_storage() {
  storage result{.fallback = 17};
  std::start_lifetime(result.elements);
  std::construct_at(&result.elements[1], value{20});
  std::construct_at(&result.elements[3], value{22});
  return result;
}

constexpr storage make_destroyed_storage() {
  storage result;
  std::start_lifetime(result.elements);
  std::construct_at(&result.elements[1], value{20});
  std::construct_at(&result.elements[3], value{22});
  std::destroy_at(&result.elements[1]);
  return result;
}

constexpr storage make_assigned_storage() {
  storage result;
  std::start_lifetime(result.elements);
  result.elements[2] = value{42};
  return result;
}

constexpr duplicate_storage make_duplicate_storage() {
  duplicate_storage result;
  std::start_lifetime(result.second);
  std::construct_at(&result.second, duplicate_value{42});
  return result;
}

constexpr duplicate_storage make_placed_duplicate_storage() {
  duplicate_storage result;
  std::construct_at(&result.second, duplicate_value{17});
  return result;
}

constexpr storage stored = make_storage();
constexpr storage dirty_stored = make_dirty_storage();
constexpr storage destroyed_stored = make_destroyed_storage();
constexpr storage assigned_stored = make_assigned_storage();
constexpr duplicate_storage duplicate_stored = make_duplicate_storage();
constexpr duplicate_storage placed_duplicate_stored =
    make_placed_duplicate_storage();
static_assert(stored.elements[1].number + stored.elements[3].number == 42);
static_assert(duplicate_stored.second.number == 42);
static_assert(placed_duplicate_stored.second.number == 17);
static_assert(destroyed_stored.elements[3].number == 22);
static_assert(assigned_stored.elements[2].number == 42);

template <storage Value>
struct token {};

using first_token = token<stored>;
using second_token = token<dirty_stored>;
using destroyed_token = token<destroyed_stored>;
static_assert(std::is_same_v<first_token, second_token>);
