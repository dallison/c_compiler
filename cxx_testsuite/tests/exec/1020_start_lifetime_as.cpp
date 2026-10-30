// RUN: -std=c++23
// EXPECT_EXIT: 0

#include <memory>
#include <stddef.h>
#include <type_traits>
#include <version>

#if __cpp_lib_start_lifetime_as != 202207L
#error "__cpp_lib_start_lifetime_as must be 202207L"
#endif
#if __cpp_lib_is_implicit_lifetime != 202302L
#error "__cpp_lib_is_implicit_lifetime must be 202302L"
#endif

struct Pair {
  int first;
  int second;
};

struct UserCtor {
  UserCtor() {}
  int value;
};

struct NotImplicit {
  NotImplicit();
  ~NotImplicit();
  int value;
};

enum Color { kRed };

struct Holder {
  int value;
};

static_assert(std::is_implicit_lifetime<int>::value);
static_assert(std::is_implicit_lifetime<const int>::value);
static_assert(std::is_implicit_lifetime<volatile int>::value);
static_assert(std::is_implicit_lifetime<int*>::value);
static_assert(std::is_implicit_lifetime<int Holder::*>::value);
static_assert(std::is_implicit_lifetime<std::nullptr_t>::value);
static_assert(std::is_implicit_lifetime<Color>::value);
static_assert(std::is_implicit_lifetime<Pair>::value);
static_assert(std::is_implicit_lifetime<const Pair>::value);
static_assert(std::is_implicit_lifetime<UserCtor>::value);
static_assert(std::is_implicit_lifetime<int[4]>::value);
static_assert(!std::is_implicit_lifetime<NotImplicit>::value);
static_assert(!std::is_implicit_lifetime<int&>::value);

int main() {
  alignas(int) unsigned char bytes[sizeof(int)] = {1, 0, 0, 0};
  int* value = std::start_lifetime_as<int>(bytes);
  if (value == nullptr || *value != 1) {
    return 1;
  }
  *value = 9;
  if (bytes[0] != 9) {
    return 2;
  }

  alignas(Pair) unsigned char pair_bytes[sizeof(Pair)] = {};
  pair_bytes[0] = 2;
  pair_bytes[4] = 3;
  Pair* pair = std::start_lifetime_as<Pair>(pair_bytes);
  if (pair->first != 2 || pair->second != 3) {
    return 3;
  }

  int raw[4] = {4, 5, 5, 6};
  int* many = std::start_lifetime_as_array<int>(raw, 4);
  if (many[0] != 4 || many[1] != 5 || many[3] != 6) {
    return 4;
  }
  int* none = std::start_lifetime_as_array<int>(raw, 0);
  if (none != static_cast<int*>(static_cast<void*>(raw))) {
    return 5;
  }

  const unsigned char stored[sizeof(int)] = {7, 0, 0, 0};
  const int* viewed = std::start_lifetime_as<const int>(stored);
  if (*viewed != 7) {
    return 6;
  }

  volatile unsigned char volatile_bytes[sizeof(int)] = {8, 0, 0, 0};
  volatile int* volatile_value =
      std::start_lifetime_as<volatile int>(volatile_bytes);
  if (*volatile_value != 8) {
    return 7;
  }

  const volatile unsigned char cv_bytes[sizeof(int)] = {11, 0, 0, 0};
  const volatile int* cv_value =
      std::start_lifetime_as<const volatile int>(cv_bytes);
  if (*cv_value != 11) {
    return 8;
  }

  const int consts[2] = {12, 13};
  const int* const_array =
      std::start_lifetime_as_array<const int>(consts, 2);
  if (const_array[0] != 12 || const_array[1] != 13) {
    return 9;
  }
  return 0;
}
