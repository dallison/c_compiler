// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <unordered_set>
#include <string>
#include <utility>
#include <initializer_list>

int basic_insert_lookup() {
  std::unordered_set<int> s;
  if (!s.empty() || s.size() != 0) {
    return 1;
  }
  std::pair<std::unordered_set<int>::iterator, bool> a = s.insert(5);
  if (!a.second || *a.first != 5) {
    return 2;
  }
  s.insert(2);
  s.insert(8);
  std::pair<std::unordered_set<int>::iterator, bool> dup = s.insert(5);
  if (dup.second || *dup.first != 5) {
    return 3;
  }
  if (s.size() != 3 || s.empty()) {
    return 4;
  }
  if (!s.contains(2) || !s.contains(5) || !s.contains(8) || s.contains(9)) {
    return 5;
  }
  if (s.count(5) != 1 || s.count(9) != 0) {
    return 6;
  }
  if (s.find(8) == s.end() || *s.find(8) != 8 || s.find(9) != s.end()) {
    return 7;
  }
  return 0;
}

int iteration_sum() {
  std::unordered_set<int> s = {5, 1, 4, 2, 3, 1, 4};
  if (s.size() != 5) {
    return 10;
  }
  int sum = 0;
  for (std::unordered_set<int>::iterator it = s.begin(); it != s.end(); ++it) {
    sum += *it;
  }
  if (sum != 15) {
    return 11;
  }
  int range_sum = 0;
  for (int x : s) {
    range_sum += x;
  }
  if (range_sum != 15) {
    return 12;
  }
  return 0;
}

int erase_variants() {
  std::unordered_set<int> s = {1, 2, 3, 4, 5};
  if (s.erase(3) != 1 || s.erase(99) != 0) {
    return 20;
  }
  if (s.size() != 4 || s.contains(3)) {
    return 21;
  }
  std::unordered_set<int>::iterator it = s.find(4);
  s.erase(it);
  if (s.contains(4) || s.size() != 3) {
    return 22;
  }
  s.clear();
  if (!s.empty()) {
    return 23;
  }
  return 0;
}

int emplace_and_dedup() {
  std::unordered_set<int> s;
  std::pair<std::unordered_set<int>::iterator, bool> e = s.emplace(7);
  if (!e.second || *e.first != 7) {
    return 30;
  }
  std::pair<std::unordered_set<int>::iterator, bool> e2 = s.emplace(7);
  if (e2.second || *e2.first != 7) {
    return 31;
  }
  if (s.size() != 1) {
    return 32;
  }
  return 0;
}

int string_elements() {
  std::unordered_set<std::string> s;
  s.insert("hello");
  s.insert("world");
  s.insert("hello");  // duplicate
  if (s.size() != 2) {
    return 40;
  }
  if (!s.contains("hello") || !s.contains("world") || s.contains("nope")) {
    return 41;
  }
  return 0;
}

int copy_and_move() {
  std::unordered_set<int> s = {1, 2, 3};
  std::unordered_set<int> copy = s;
  if (copy.size() != 3 || !copy.contains(2)) {
    return 50;
  }
  copy.insert(4);
  if (s.contains(4)) {
    return 51;  // deep copy
  }
  std::unordered_set<int> moved = std::move(copy);
  if (moved.size() != 4 || !moved.contains(4)) {
    return 52;
  }
  return 0;
}

int rehash_growth() {
  std::unordered_set<int> s;
  for (int i = 0; i < 300; ++i) {
    s.insert(i);
  }
  if (s.size() != 300) {
    return 60;
  }
  for (int i = 0; i < 300; ++i) {
    if (!s.contains(i)) {
      return 61;
    }
  }
  // inserting duplicates does not grow
  for (int i = 0; i < 300; ++i) {
    s.insert(i);
  }
  if (s.size() != 300) {
    return 62;
  }
  return 0;
}

int main(void) {
  int rc;
  if ((rc = basic_insert_lookup()) != 0) return rc;
  if ((rc = iteration_sum()) != 0) return rc;
  if ((rc = erase_variants()) != 0) return rc;
  if ((rc = emplace_and_dedup()) != 0) return rc;
  if ((rc = string_elements()) != 0) return rc;
  if ((rc = copy_and_move()) != 0) return rc;
  if ((rc = rehash_growth()) != 0) return rc;
  return 0;
}
