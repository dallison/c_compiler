// RUN: -std=c++26

#include <hive>

constexpr bool empty_hive_limits_work() {
  constexpr std::hive_limits limits(4, 32);
  std::hive<int> values(limits);
  return values.empty() && values.block_capacity_limits().min == 4 &&
         values.block_capacity_limits().max == 32 &&
         std::hive<int>::is_within_hard_limits(limits);
}

static_assert(empty_hive_limits_work());

constexpr bool empty_hive_is_empty() {
  std::hive<int> values(std::hive_limits(4, 32));
  return values.empty();
}

constexpr bool empty_hive_minimum_is_preserved() {
  std::hive<int> values(std::hive_limits(4, 32));
  return values.block_capacity_limits().min == 4;
}

constexpr bool empty_hive_maximum_is_preserved() {
  std::hive<int> values(std::hive_limits(4, 32));
  return values.block_capacity_limits().max == 32;
}

static_assert(empty_hive_is_empty());
static_assert(empty_hive_minimum_is_preserved());
static_assert(empty_hive_maximum_is_preserved());
static_assert(
    std::hive<int>::is_within_hard_limits(std::hive_limits(4, 32)));
static_assert(std::hive_limits(4, 32).min == 4);
static_assert(std::hive_limits(4, 32).max == 32);
static_assert(std::hive<int>::block_capacity_hard_limits().max >= 32);

constexpr bool limits_after_empty() {
  std::hive<int> values(std::hive_limits(4, 32));
  bool empty = values.empty();
  return empty && values.block_capacity_limits().min == 4;
}

constexpr bool maximum_after_minimum() {
  std::hive<int> values(std::hive_limits(4, 32));
  bool minimum = values.block_capacity_limits().min == 4;
  return minimum && values.block_capacity_limits().max == 32;
}

static_assert(limits_after_empty());
static_assert(maximum_after_minimum());
