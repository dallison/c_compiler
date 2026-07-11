// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <map>
#include <utility>

struct LongKey {
  long value;

  LongKey(long v) : value(v) {}
};

bool operator<(const LongKey& left, const LongKey& right) {
  return left.value < right.value;
}

bool operator<(const LongKey& left, long right) {
  return left.value < right;
}

bool operator<(long left, const LongKey& right) {
  return left < right.value;
}

struct RemoveEven {
  bool operator()(const std::pair<const int, int>& value) const {
    return (value.first % 2) == 0;
  }
};

int observers_and_heterogeneous_lookup() {
  std::map<LongKey, int, std::less<void> > values;
  values.insert(std::pair<const LongKey, int>(LongKey(2), 20));
  values.insert(std::pair<const LongKey, int>(LongKey(4), 40));

  if (!values.key_comp()(LongKey(1), LongKey(2))) {
    return 1;
  }
  if (!values.value_comp()(*values.find(2L), *values.find(4L))) {
    return 2;
  }
  if (values.find(2L)->second != 20 || !values.contains(4L) ||
      values.count(3L) != 0) {
    return 3;
  }
  if (values.lower_bound(3L)->first.value != 4 ||
      values.upper_bound(2L)->first.value != 4) {
    return 4;
  }
  std::pair<std::map<LongKey, int, std::less<void> >::iterator,
            std::map<LongKey, int, std::less<void> >::iterator> range =
      values.equal_range(2L);
  if (range.first->second != 20 || range.second->second != 40) {
    return 5;
  }
  if (values.erase(2L) != 1 || values.contains(2L)) {
    return 6;
  }
  return 0;
}

int erase_and_compare() {
  std::map<int, int> values;
  values.insert(std::pair<const int, int>(1, 10));
  values.insert(std::pair<const int, int>(2, 20));
  values.insert(std::pair<const int, int>(3, 30));
  values.insert(std::pair<const int, int>(4, 40));

  std::map<int, int>::const_iterator first = values.find(1);
  values.erase(first);
  if (values.contains(1) || values.size() != 3) {
    return 1;
  }

  values.erase(values.find(2), values.find(4));
  if (values.size() != 1 || values.begin()->first != 4) {
    return 2;
  }

  values.insert(std::pair<const int, int>(2, 20));
  values.insert(std::pair<const int, int>(3, 30));
  if (std::erase_if(values, RemoveEven()) != 2 || values.size() != 1 ||
      values.begin()->first != 3) {
    return 3;
  }

  std::map<int, int> same;
  same.insert(std::pair<const int, int>(3, 30));
  std::map<int, int> larger;
  larger.insert(std::pair<const int, int>(3, 31));
  if (!(values == same) || (values == larger)) {
    return 4;
  }
  if (!((values <=> larger) < 0) || !((larger <=> values) > 0) ||
      !((values <=> same) == 0)) {
    return 5;
  }
  return 0;
}

int main() {
  if (observers_and_heterogeneous_lookup() != 0) {
    return 1;
  }
  if (erase_and_compare() != 0) {
    return 2;
  }
  return 0;
}
