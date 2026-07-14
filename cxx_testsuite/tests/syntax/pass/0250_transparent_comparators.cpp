// RUN: -std=c++20

#include <functional>
#include <map>
#include <type_traits>

static_assert(std::is_same<std::less<>, std::less<void>>::value);
static_assert(std::is_same<std::greater<>, std::greater<void>>::value);
static_assert(std::is_same<std::equal_to<>, std::equal_to<void>>::value);

using LessTransparent = typename std::less<>::is_transparent;
using GreaterTransparent = typename std::greater<>::is_transparent;
using EqualTransparent = typename std::equal_to<>::is_transparent;
using TransparentVoid = std::void_t<typename std::less<>::is_transparent>;

static_assert(std::is_same<LessTransparent, void>::value);
static_assert(std::is_same<GreaterTransparent, void>::value);
static_assert(std::is_same<EqualTransparent, void>::value);
static_assert(std::is_same<TransparentVoid, void>::value);

struct Key {
  int value;
};

struct Probe {
  int value;
};

struct NonTransparentCompare {
  bool operator()(const Key&, const Key&) const;
  bool operator()(const Key&, const Probe&) const;
  bool operator()(const Probe&, const Key&) const;
};

struct TransparentCompare : NonTransparentCompare {
  using is_transparent = void;
};

static_assert(
    std::__map_detail::__is_transparent_compare<TransparentCompare>::value,
    "transparent comparator detection");
static_assert(
    !std::__map_detail::__is_transparent_compare<NonTransparentCompare>::value,
    "non-transparent comparator rejection");

void check_heterogeneous_lookup(
    std::map<Key, int, TransparentCompare>& map, const Probe& probe) {
  map.find(probe);
  map.contains(probe);
  map.count(probe);
  map.lower_bound(probe);
  map.upper_bound(probe);
  map.equal_range(probe);
}
