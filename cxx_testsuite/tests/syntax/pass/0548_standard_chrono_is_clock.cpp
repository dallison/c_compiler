// RUN: -std=c++20

#include <chrono>

struct user_clock {
  using rep = long long;
  using period = std::ratio<1, 1000>;
  using duration = std::chrono::duration<rep, period>;
  using time_point = std::chrono::time_point<user_clock, duration>;
  static constexpr bool is_steady = true;
  static time_point now() noexcept;
};

struct not_a_clock {};

struct missing_now {
  using rep = long long;
  using period = std::ratio<1>;
  using duration = std::chrono::seconds;
  using time_point = std::chrono::time_point<missing_now>;
  static constexpr bool is_steady = false;
};

struct missing_is_steady {
  using rep = long long;
  using period = std::ratio<1>;
  using duration = std::chrono::seconds;
  using time_point = std::chrono::time_point<missing_is_steady>;
  static time_point now() noexcept;
};

template <class T>
using steady_member_type = decltype(T::is_steady);

static_assert(
    std::is_same_v<steady_member_type<user_clock>, const bool>);
static_assert(std::chrono::is_clock<std::chrono::system_clock>::value);
static_assert(std::chrono::is_clock_v<std::chrono::steady_clock>);
static_assert(std::chrono::is_clock_v<user_clock>);
static_assert(!std::chrono::is_clock_v<not_a_clock>);
static_assert(!std::chrono::is_clock_v<missing_now>);
static_assert(!std::chrono::is_clock_v<missing_is_steady>);
static_assert(!std::chrono::is_clock_v<int>);

int main() { return 0; }
