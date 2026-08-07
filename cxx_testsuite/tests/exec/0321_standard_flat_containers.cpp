// RUN: -std=c++23
// EXPECT_EXIT: 0

#include <flat_map>
#include <flat_set>
#include <vector>
#include <version>

int main() {
  std::flat_set<int> set = {4, 1, 3, 1, 2};
  if (set.size() != 4) {
    return 20 + set.size();
  }
  if (*set.begin() != 1) {
    return 16;
  }
  if (*set.rbegin() != 4) {
    return 17;
  }
  auto duplicate = set.insert(3);
  auto inserted = set.insert(5);
  if (duplicate.second || !inserted.second || set.contains(7) ||
      set.count(3) != 1 || set.lower_bound(3) - set.begin() != 2) {
    return 2;
  }
  set.insert_range(std::vector<int>{0, 6, 4});
  if (set.size() != 7 || set.erase(4) != 1 ||
      std::erase_if(set, [](int value) { return value % 2 != 0; }) != 3) {
    return 3;
  }
  auto set_keys = std::move(set).extract();
  std::flat_set<int> restored_set(std::sorted_unique,
                                  std::move(set_keys));
  if (restored_set.size() != 3 || !restored_set.contains(0) ||
      !restored_set.contains(2) || !restored_set.contains(6)) {
    return 4;
  }

  std::flat_multiset<int> multiset = {2, 1, 2, 3, 2};
  if (multiset.size() != 5 || multiset.count(2) != 3) {
    return 5;
  }
  multiset.insert(2);
  auto set_range = multiset.equal_range(2);
  if (set_range.second - set_range.first != 4 ||
      multiset.erase(2) != 4 || multiset.contains(2)) {
    return 6;
  }

  std::vector<int> range_keys = {9, 7, 8, 7};
  std::flat_set<int> range_set(std::from_range, range_keys);
  if (range_set.size() != 3 || *range_set.begin() != 7) {
    return 7;
  }

  std::flat_map<int, int> map = {{3, 30}, {1, 10}, {2, 20}, {1, 11}};
  auto initial_three = map.find(3);
  if (map.size() != 3 || map.begin()->first != 1 ||
      map.begin()->second != 10 || initial_three == map.end() ||
      initial_three->second != 30) {
    return 8;
  }
  map[4] = 40;
  auto emplaced = map.try_emplace(5, 50);
  auto duplicate_map = map.try_emplace(5, 55);
  auto assigned = map.insert_or_assign(2, 22);
  auto inserted_map = map.insert_or_assign(6, 60);
  auto found_two = map.find(2);
  auto found_five = map.find(5);
  if (!emplaced.second || duplicate_map.second || assigned.second ||
      !inserted_map.second || found_two == map.end() ||
      found_two->second != 22 || found_five == map.end() ||
      found_five->second != 50) {
    return 9;
  }
  auto found = map.find(3);
  found->second = 33;
  map.insert_range(
      std::vector<std::pair<int, int>>{{7, 70}, {3, 300}});
  auto found_seven = map.find(7);
  if (map.find(3)->second != 33 || found_seven == map.end() ||
      found_seven->second != 70 || map.erase(4) != 1 || map.contains(4)) {
    return 10;
  }
  if (std::erase_if(map, [](auto value) { return value.first > 5; }) != 2 ||
      map.size() != 4) {
    return 11;
  }
  auto map_storage = std::move(map).extract();
  if (map_storage.keys.size() != map_storage.values.size()) {
    return 12;
  }
  std::flat_map<int, int> restored_map(
      std::sorted_unique, std::move(map_storage.keys),
      std::move(map_storage.values));
  auto restored_three = restored_map.find(3);
  if (restored_map.size() != 4 || restored_three == restored_map.end() ||
      restored_three->second != 33) {
    return 13;
  }

  std::flat_multimap<int, int> multimap = {
      {2, 20}, {1, 10}, {2, 21}, {2, 22}};
  if (multimap.size() != 4 || multimap.count(2) != 3) {
    return 14;
  }
  multimap.insert(std::pair<int, int>{2, 23});
  auto map_range = multimap.equal_range(2);
  int sum = 0;
  for (auto iterator = map_range.first; iterator != map_range.second;
       ++iterator) {
    sum += iterator->second;
  }
  if (map_range.second - map_range.first != 4 || sum != 86 ||
      multimap.erase(2) != 4 || multimap.size() != 1) {
    return 15;
  }

  return 0;
}
