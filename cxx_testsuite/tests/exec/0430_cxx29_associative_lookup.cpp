// RUN: -std=c++29
// EXPECT_EXIT: 0

#include <concepts>
#include <flat_map>
#include <map>
#include <optional>
#include <type_traits>
#include <unordered_map>
#include <version>

#if __cpp_lib_map_lookup != 202606L
#error "__cpp_lib_map_lookup has the wrong value"
#endif

struct key {
  int value;

  friend constexpr bool operator==(key, key) = default;
};

struct probe {
  int value;
};

struct transparent_less {
  using is_transparent = void;

  constexpr bool operator()(key left, key right) const {
    return left.value < right.value;
  }
  constexpr bool operator()(key left, probe right) const {
    return left.value < right.value;
  }
  constexpr bool operator()(probe left, key right) const {
    return left.value < right.value;
  }
};

struct opaque_less {
  constexpr bool operator()(key left, key right) const {
    return left.value < right.value;
  }
};

struct transparent_hash {
  using is_transparent = void;

  constexpr unsigned long operator()(key value) const {
    return static_cast<unsigned long>(value.value);
  }
  constexpr unsigned long operator()(probe value) const {
    return static_cast<unsigned long>(value.value);
  }
};

struct transparent_equal {
  using is_transparent = void;

  constexpr bool operator()(key left, key right) const {
    return left.value == right.value;
  }
  constexpr bool operator()(key left, probe right) const {
    return left.value == right.value;
  }
};

template <class Container>
concept has_probe_lookup =
    requires(Container& container) { container.lookup(probe{1}); };

using ordered = std::map<key, int, transparent_less>;
using unordered =
    std::unordered_map<key, int, transparent_hash, transparent_equal>;
using flat = std::flat_map<key, int, transparent_less>;

static_assert(
    std::same_as<decltype(std::declval<ordered&>().lookup(key{1})),
                 std::optional<int&>>);
static_assert(
    std::same_as<decltype(std::declval<const ordered&>().lookup(key{1})),
                 std::optional<const int&>>);
static_assert(has_probe_lookup<ordered>);
static_assert(has_probe_lookup<unordered>);
static_assert(has_probe_lookup<flat>);
static_assert(!has_probe_lookup<std::map<key, int, opaque_less>>);
static_assert(!has_probe_lookup<std::flat_map<key, int, opaque_less>>);
static_assert(!has_probe_lookup<std::flat_multimap<key, int, transparent_less>>);

template <class Container>
int test_lookup(Container& container) {
  container.insert({key{1}, 10});
  container.insert({key{2}, 20});

  std::optional<int&> found = container.lookup(probe{2});
  if (!found || *found != 20 || &*found != &container.at(key{2})) {
    return 1;
  }
  *found = 25;
  if (container.at(key{2}) != 25) {
    return 2;
  }
  if (container.lookup(probe{3}).has_value()) {
    return 3;
  }

  const Container& constant = container;
  std::optional<const int&> constant_found = constant.lookup(probe{1});
  if (!constant_found || *constant_found != 10 ||
      &*constant_found != &constant.at(key{1})) {
    return 4;
  }
  if (constant.lookup(probe{4}).value_or(40) != 40) {
    return 5;
  }
  return 0;
}

int main() {
  ordered tree;
  int result = test_lookup(tree);
  if (result != 0) {
    return result;
  }

  unordered hash;
  result = test_lookup(hash);
  if (result != 0) {
    return result + 10;
  }

  flat sorted;
  result = test_lookup(sorted);
  if (result != 0) {
    return result + 20;
  }
  return 0;
}
