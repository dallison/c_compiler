// RUN: -std=c++26

#include <array>
#include <hive>
#include <memory_resource>
#include <type_traits>
#include <version>

#if __cpp_lib_hive != 202502L
#error "__cpp_lib_hive has the wrong value"
#endif

using hive_type = std::hive<int>;

static_assert(std::is_same_v<hive_type::value_type, int>);
static_assert(std::is_same_v<hive_type::allocator_type, std::allocator<int>>);
static_assert(std::is_same_v<hive_type::reference, int&>);
static_assert(std::is_same_v<hive_type::const_reference, const int&>);
static_assert(std::is_same_v<hive_type::iterator::iterator_category,
                             std::bidirectional_iterator_tag>);
static_assert(std::is_same_v<
              decltype(std::declval<hive_type::iterator>() <=>
                       std::declval<hive_type::const_iterator>()),
              std::strong_ordering>);

constexpr bool limits_work() {
  constexpr std::hive_limits limits(4, 32);
  std::hive<int> values(limits);
  return values.empty() && values.block_capacity_limits().min == 4 &&
         values.block_capacity_limits().max == 32 &&
         std::hive<int>::is_within_hard_limits(limits);
}

static_assert(limits_work());

struct move_only {
  int value;
  move_only(int v) : value(v) {}
  move_only(const move_only&) = delete;
  move_only(move_only&&) = default;
  move_only& operator=(const move_only&) = delete;
  move_only& operator=(move_only&&) = default;
  bool operator<(const move_only& other) const {
    return value < other.value;
  }
};

void exercise_api() {
  int source[] = {1, 2, 3};
  std::array<int, 3> range{4, 5, 6};

  hive_type empty;
  hive_type with_allocator(std::allocator<int>());
  hive_type with_limits(std::hive_limits(2, 16), std::allocator<int>());
  hive_type counted(3);
  hive_type filled(3, 7);
  hive_type iterated(source, source + 3);
  hive_type iterated_limits(source, source + 3, std::hive_limits(2, 16));
  hive_type ranged(std::from_range, range);
  hive_type ranged_limits(std::from_range, range, std::hive_limits(2, 16));
  hive_type initialized{1, 2, 3};
  hive_type initialized_limits({1, 2}, std::hive_limits(2, 16));
  hive_type copied(initialized);
  hive_type copied_alloc(copied, std::allocator<int>());
  hive_type moved(static_cast<hive_type&&>(copied));
  hive_type moved_alloc(static_cast<hive_type&&>(moved),
                        std::allocator<int>());

  empty = initialized;
  empty = static_cast<hive_type&&>(moved_alloc);
  empty = {8, 9};
  empty.assign(source, source + 3);
  empty.assign_range(range);
  empty.assign(2, 4);
  empty.assign({5, 6});

  auto inserted = empty.insert(1);
  empty.insert(static_cast<int&&>(*inserted));
  empty.insert(empty.cbegin(), 2);
  empty.insert(empty.cend(), 3);
  empty.emplace(4);
  empty.emplace_hint(empty.cbegin(), 5);
  empty.insert({6, 7});
  empty.insert_range(range);
  empty.insert(source, source + 3);
  empty.insert(2, 8);
  empty.erase(empty.cbegin());
  empty.erase(empty.cbegin(), empty.cend());

  empty.reserve(32);
  empty.shrink_to_fit();
  empty.trim_capacity();
  empty.trim_capacity(4);
  empty.reshape(std::hive_limits(2, 32));
  empty.swap(initialized);
  std::swap(empty, initialized);
  empty.splice(initialized);
  empty.splice(static_cast<hive_type&&>(initialized));
  empty.unique();
  empty.unique([](int left, int right) { return left == right; });
  empty.sort();
  empty.sort([](int left, int right) { return left < right; });
  if (!empty.empty()) {
    int* pointer = &*empty.begin();
    (void)empty.get_iterator(pointer);
    const hive_type& constant = empty;
    (void)constant.get_iterator(pointer);
  }
  std::erase(empty, 2);
  std::erase_if(empty, [](int value) { return value == 3; });
  empty.clear();

  std::hive<move_only> noncopyable;
  noncopyable.emplace(2);
  noncopyable.emplace(1);
  noncopyable.sort();

  std::pmr::hive<int> polymorphic;
  polymorphic.emplace(1);
}

auto deduced_iterators(int* first, int* last) {
  return std::hive(first, last);
}

auto deduced_range(std::array<long, 2>& values) {
  return std::hive(std::from_range, values);
}
