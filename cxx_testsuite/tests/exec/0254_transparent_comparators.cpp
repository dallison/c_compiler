// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <functional>
#include <map>
#include <utility>

struct Key {
  int value;
};

struct Probe {
  int value;
};

bool operator<(const Key& left, const Key& right) {
  return left.value < right.value;
}

bool operator<(const Key& left, const Probe& right) {
  return left.value < right.value;
}

bool operator<(const Probe& left, const Key& right) {
  return left.value < right.value;
}

int main() {
  std::less<> less;
  std::greater<> greater;
  std::equal_to<> equal;
  if (!less(1, 2L) || !greater(3L, 2) || !equal(4, 4L)) {
    return 1;
  }

  std::map<Key, int, std::less<> > values;
  values.insert(std::pair<const Key, int>(Key{1}, 10));
  values.insert(std::pair<const Key, int>(Key{3}, 30));
  values.insert(std::pair<const Key, int>(Key{5}, 50));

  const Probe three{3};
  std::map<Key, int, std::less<> >::iterator found = values.find(three);
  if (found == values.end() || found->second != 30) {
    return 2;
  }
  if (!values.contains(three) || values.count(three) != 1) {
    return 3;
  }

  std::map<Key, int, std::less<> >::iterator lower =
      values.lower_bound(Probe{2});
  if (lower == values.end() || lower->first.value != 3) {
    return 4;
  }
  std::map<Key, int, std::less<> >::iterator upper =
      values.upper_bound(three);
  if (upper == values.end() || upper->first.value != 5) {
    return 5;
  }

  std::pair<std::map<Key, int, std::less<> >::iterator,
            std::map<Key, int, std::less<> >::iterator> range =
      values.equal_range(three);
  if (range.first == values.end() || range.first->first.value != 3 ||
      range.second == values.end() || range.second->first.value != 5) {
    return 6;
  }
  return 0;
}
