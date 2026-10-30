// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <unordered_map>
#include <unordered_set>

static bool is_two(int value) { return value == 2; }

static bool mapped_is_six(const std::pair<const int, int>& value) {
  return value.second == 6;
}

static int multiset_range(const std::unordered_multiset<int>& values, int key) {
  std::pair<std::unordered_multiset<int>::const_iterator,
            std::unordered_multiset<int>::const_iterator>
      range = values.equal_range(key);
  int count = 0;
  for (std::unordered_multiset<int>::const_iterator it = range.first;
       it != range.second; ++it) {
    if (*it != key) {
      return -1;
    }
    ++count;
  }
  return count;
}

int main() {
  std::unordered_multiset<int> values;
  values.insert(1);
  values.insert(2);
  values.insert(1);
  values.emplace(1);
  if (values.size() != 4 || values.count(1) != 3 || values.count(2) != 1) {
    return 1;
  }
  if (multiset_range(values, 1) != 3 || multiset_range(values, 2) != 1) {
    return 2;
  }
  if (values.find(1) == values.end() || *values.find(1) != 1) {
    return 3;
  }

  std::unordered_multiset<int>::iterator one = values.find(1);
  values.erase(one);
  if (values.count(1) != 2 || values.size() != 3) {
    return 4;
  }
  if (values.erase(1) != 2 || values.count(1) != 0 || values.count(2) != 1 ||
      values.size() != 1) {
    return 5;
  }

  std::unordered_multiset<int> copy = values;
  copy.insert(2);
  if (values.count(2) != 1 || copy.count(2) != 2 || !(values == values)) {
    return 6;
  }
  std::unordered_multiset<int> same;
  same.insert(2);
  if (!(values == same)) {
    return 7;
  }
  same.insert(2);
  if (values == same) {
    return 8;
  }

  std::unordered_multiset<int> crowded = {1, 1, 2};
  for (int key = 3; key < 40; ++key) {
    crowded.insert(key);
  }
  crowded.insert(1);
  if (crowded.count(1) != 3 || multiset_range(crowded, 1) != 3 ||
      crowded.count(2) != 1 || crowded.count(39) != 1) {
    return 9;
  }
  std::unordered_multiset<int> crowded_copy = crowded;
  if (crowded_copy.count(1) != 3 || multiset_range(crowded_copy, 1) != 3 ||
      crowded_copy.size() != crowded.size() || !(crowded == crowded_copy)) {
    return 10;
  }
  if (crowded.erase(1) != 3 || crowded.count(1) != 0 || crowded.count(39) != 1 ||
      crowded_copy.count(1) != 3) {
    return 11;
  }

  std::unordered_set<int> unique;
  unique.insert(4);
  std::pair<std::unordered_set<int>::iterator, bool> rejected = unique.insert(4);
  if (rejected.second || unique.size() != 1 || unique.count(4) != 1) {
    return 19;
  }
  std::pair<std::unordered_set<int>::iterator, std::unordered_set<int>::iterator>
      unique_range = unique.equal_range(4);
  int unique_count = 0;
  for (std::unordered_set<int>::iterator it = unique_range.first;
       it != unique_range.second; ++it) {
    ++unique_count;
  }
  if (unique_count != 1 || unique.erase(4) != 1 || !unique.empty()) {
    return 12;
  }

  std::unordered_multimap<int, int> mapped;
  mapped.insert(std::pair<const int, int>(5, 6));
  mapped.insert(std::pair<const int, int>(5, 7));
  mapped.emplace(8, 9);
  if (mapped.size() != 3 || mapped.count(5) != 2 || mapped.count(8) != 1) {
    return 13;
  }
  std::pair<std::unordered_multimap<int, int>::iterator,
            std::unordered_multimap<int, int>::iterator>
      mapped_range = mapped.equal_range(5);
  int seen_six = 0;
  int seen_seven = 0;
  int mapped_count = 0;
  for (std::unordered_multimap<int, int>::iterator it = mapped_range.first;
       it != mapped_range.second; ++it) {
    ++mapped_count;
    if (it->second == 6) {
      ++seen_six;
    } else if (it->second == 7) {
      ++seen_seven;
    }
  }
  if (mapped_count != 2 || seen_six != 1 || seen_seven != 1) {
    return 14;
  }
  if (mapped.erase(5) != 2 || mapped.count(5) != 0 || mapped.count(8) != 1 ||
      mapped.size() != 1) {
    return 15;
  }

  std::unordered_multimap<int, int> mapped_copy = mapped;
  mapped_copy.insert(std::pair<const int, int>(8, 10));
  if (mapped.count(8) != 1 || mapped_copy.count(8) != 2 ||
      !(mapped == mapped)) {
    return 16;
  }
  if (mapped == mapped_copy) {
    return 17;
  }

  std::unordered_map<int, int> one_value;
  one_value.insert(std::pair<const int, int>(1, 2));
  std::pair<std::unordered_map<int, int>::iterator, bool> kept =
      one_value.insert(std::pair<const int, int>(1, 3));
  if (kept.second || one_value.size() != 1 || one_value.count(1) != 1 ||
      one_value.find(1)->second != 2 || one_value.erase(1) != 1) {
    return 18;
  }

  std::unordered_multiset<int> filtered;
  filtered.insert(1);
  filtered.insert(2);
  filtered.insert(2);
  filtered.insert(3);
  if (std::erase_if(filtered, is_two) != 2 || filtered.count(2) != 0 ||
      filtered.count(1) != 1 || filtered.count(3) != 1) {
    return 20;
  }

  std::unordered_multimap<int, int> filtered_map;
  filtered_map.insert(std::pair<const int, int>(1, 6));
  filtered_map.insert(std::pair<const int, int>(1, 7));
  filtered_map.insert(std::pair<const int, int>(2, 6));
  if (std::erase_if(filtered_map, mapped_is_six) != 2 ||
      filtered_map.count(1) != 1 || filtered_map.count(2) != 0) {
    return 21;
  }
  return 0;
}
