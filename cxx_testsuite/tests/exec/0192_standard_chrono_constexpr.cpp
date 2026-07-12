// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <chrono>

using namespace std::chrono;
using namespace std::chrono_literals;

constexpr milliseconds constexpr_duration_sum(void) {
  return 2s + 250ms;
}

constexpr long long constexpr_duration_mutation(void) {
  milliseconds value(1);
  value += 2ms;
  ++value;
  value *= 3;
  return value.count();
}

constexpr long long constexpr_duration_cast_value(void) {
  return duration_cast<microseconds>(2ms + 500us).count();
}

constexpr long long constexpr_time_point_value(void) {
  time_point<system_clock, milliseconds> epoch(10s);
  auto later = epoch + 250ms;
  return (later - epoch).count() +
         time_point_cast<seconds>(later).time_since_epoch().count();
}

consteval milliseconds immediate_duration(long long value) {
  return milliseconds(value) + 1ms;
}

consteval long long immediate_duration_count(void) {
  return (3s + 125ms).count();
}

constexpr milliseconds global_duration = constexpr_duration_sum();
constexpr milliseconds global_immediate_duration = immediate_duration(41);

static_assert(global_duration.count() == 2250,
              "constexpr duration arithmetic");
static_assert(constexpr_duration_mutation() == 12,
              "constexpr duration mutation");
static_assert(constexpr_duration_cast_value() == 2500,
              "constexpr duration_cast");
static_assert(constexpr_time_point_value() == 260,
              "constexpr time_point arithmetic");
static_assert(global_immediate_duration.count() == 42,
              "consteval duration object return");
static_assert(immediate_duration_count() == 3125,
              "consteval duration arithmetic");
static_assert(1min == 60s, "constexpr duration comparison");

int main(void) {
  return global_duration.count() == 2250 &&
                 global_immediate_duration.count() == 42
             ? 0
             : 1;
}
