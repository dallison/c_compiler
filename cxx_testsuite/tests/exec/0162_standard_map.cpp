// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <map>
#include <stdexcept>

int ordered_iteration(void) {
  std::map<int, int> values;
  values.insert(std::pair<const int, int>(3, 30));
  values.insert(std::pair<const int, int>(1, 10));
  values.insert(std::pair<const int, int>(4, 40));
  values.insert(std::pair<const int, int>(2, 20));
  int expected_key = 1;
  int sum = 0;
  for (std::map<int, int>::iterator it = values.begin(); it != values.end();
       ++it) {
    if (it->first != expected_key) {
      return 100 + expected_key;
    }
    sum += it->second;
    expected_key++;
  }
  return sum;
}

int lookup_and_insert(void) {
  std::map<int, int> values;
  std::pair<std::map<int, int>::iterator, bool> first =
      values.insert(std::pair<const int, int>(2, 20));
  std::pair<std::map<int, int>::iterator, bool> second =
      values.insert(std::pair<const int, int>(2, 99));
  if (!first.second || second.second) {
    return 1;
  }
  if (values.size() != 1 || values.find(2)->second != 20) {
    return 2;
  }
  values[4] = 40;
  values[3] = 30;
  if (!values.contains(3) || values.count(5) != 0) {
    return 3;
  }
  try {
    values.at(5);
  } catch (const std::out_of_range&) {
    return values.at(2) + values.at(3) + values.at(4);
  }
  return 4;
}

int bounds_and_ranges(void) {
  std::map<int, int> values;
  values[1] = 10;
  values[3] = 30;
  values[5] = 50;
  std::map<int, int>::iterator low = values.lower_bound(2);
  std::map<int, int>::iterator high = values.upper_bound(3);
  std::pair<std::map<int, int>::iterator, std::map<int, int>::iterator> range =
      values.equal_range(3);
  if (low == values.end() || low->first != 3) {
    return 1;
  }
  if (high == values.end() || high->first != 5) {
    return 2;
  }
  if (range.first == values.end() || range.first->second != 30 ||
      range.second == values.end() || range.second->first != 5) {
    return 3;
  }
  return 0;
}

int copy_move_swap_erase(void) {
  std::map<int, int> values;
  values[2] = 20;
  values[1] = 10;
  values[3] = 30;
  std::map<int, int> copy = values;
  if (copy.size() != 3 || copy.find(1)->second != 10) {
    return 1;
  }
  copy.erase(2);
  if (copy.contains(2) || copy.size() != 2) {
    return 2;
  }
  std::map<int, int>::iterator next = copy.erase(copy.begin());
  if (next == copy.end() || next->first != 3 || copy.size() != 1) {
    return 3;
  }
  std::map<int, int> moved = std::move(values);
  if (moved.size() != 3 || !values.empty()) {
    return 4;
  }
  moved.swap(copy);
  if (moved.size() != 1 || copy.size() != 3) {
    return 5;
  }
  return moved.begin()->second + copy.begin()->second;
}

struct Tracker {
  static int live;
  int value;

  Tracker() : value(0) { live++; }
  Tracker(int v) : value(v) { live++; }
  Tracker(const Tracker& other) : value(other.value) { live++; }
  ~Tracker() { live--; }
  Tracker& operator=(const Tracker& other) {
    value = other.value;
    return *this;
  }
};

int Tracker::live = 0;

int lifetime_api(void) {
  {
    std::map<int, Tracker> values;
    values.insert(std::pair<const int, Tracker>(1, Tracker(7)));
    values[2] = Tracker(9);
    if (Tracker::live < 2) {
      return 1;
    }
    std::map<int, Tracker> copy = values;
    if (copy.find(1)->second.value != 7 || copy.find(2)->second.value != 9) {
      return 2;
    }
  }
  return Tracker::live;
}

int main(void) {
  if (ordered_iteration() != 100) {
    return 1;
  }
  if (lookup_and_insert() != 90) {
    return 2;
  }
  if (bounds_and_ranges() != 0) {
    return 3;
  }
  if (copy_move_swap_erase() != 40) {
    return 4;
  }
  if (lifetime_api() != 0) {
    return 5;
  }
  return 0;
}
