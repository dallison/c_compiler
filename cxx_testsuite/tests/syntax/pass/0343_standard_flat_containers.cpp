// RUN: -std=c++23

#include <flat_map>
#include <flat_set>
#include <type_traits>
#include <vector>
#include <version>

#if __cpp_lib_flat_map != 202207L
#error "__cpp_lib_flat_map has the wrong value"
#endif
#if __cpp_lib_flat_set != 202207L
#error "__cpp_lib_flat_set has the wrong value"
#endif

struct transparent_less {
  using is_transparent = void;

  template <class Left, class Right>
  constexpr bool operator()(const Left& left, const Right& right) const {
    return left < right;
  }
};

void check_flat_containers() {
  std::flat_set<int> set = {3, 1, 2, 1};
  auto set_inserted = set.insert(4);
  static_assert(std::same_as<decltype(set_inserted),
                             std::pair<std::flat_set<int>::iterator, bool>>);
  set.emplace_hint(set.begin(), 0);
  set.insert_range(std::vector<int>{5, 6});
  auto set_storage = std::move(set).extract();
  std::flat_set restored(std::sorted_unique, std::move(set_storage));
  (void)restored;

  std::flat_multiset<int> multiset = {2, 1, 2};
  static_assert(std::same_as<decltype(multiset.insert(2)),
                             std::flat_multiset<int>::iterator>);
  auto multi_storage = std::move(multiset).extract();
  std::flat_multiset restored_multi(
      std::sorted_equivalent, std::move(multi_storage));
  (void)restored_multi;

  std::flat_map<int, int, transparent_less> map = {
      {3, 30}, {1, 10}, {2, 20}};
  map[4] = 40;
  map.try_emplace(5, 50);
  map.insert_or_assign(2, 22);
  map.insert_range(std::vector<std::pair<int, int>>{{6, 60}});
  auto found = map.find(3L);
  found->second = 33;
  auto map_storage = std::move(map).extract();
  std::flat_map restored_map(std::sorted_unique,
                             std::move(map_storage.keys),
                             std::move(map_storage.values),
                             transparent_less{});
  (void)restored_map;

  std::flat_multimap<int, int> multimap = {{2, 20}, {1, 10}, {2, 21}};
  static_assert(std::same_as<decltype(multimap.insert(
                                 std::pair<int, int>{2, 22})),
                             std::flat_multimap<int, int>::iterator>);
  auto range = multimap.equal_range(2);
  (void)range;
}
