// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <unordered_map>
#include <string>
#include <utility>
#include <initializer_list>

int basic_insert_lookup() {
  std::unordered_map<int, int> m;
  if (!m.empty() || m.size() != 0) {
    return 1;
  }
  std::pair<std::unordered_map<int, int>::iterator, bool> a = m.insert({1, 10});
  if (!a.second || a.first->first != 1 || a.first->second != 10) {
    return 2;
  }
  m.insert({2, 20});
  m.insert({3, 30});
  std::pair<std::unordered_map<int, int>::iterator, bool> dup = m.insert({1, 99});
  if (dup.second || dup.first->second != 10) {
    return 3;
  }
  if (m.size() != 3 || m.empty()) {
    return 4;
  }
  if (!m.contains(1) || !m.contains(2) || !m.contains(3) || m.contains(9)) {
    return 5;
  }
  if (m.count(2) != 1 || m.count(9) != 0) {
    return 6;
  }
  if (m.find(3) == m.end() || m.find(3)->second != 30 || m.find(9) != m.end()) {
    return 7;
  }
  return 0;
}

int subscript_and_at() {
  std::unordered_map<int, int> m;
  m[5] = 50;
  m[6] = 60;
  m[5] = 55;  // overwrite
  if (m.size() != 2) {
    return 10;
  }
  if (m[5] != 55 || m[6] != 60) {
    return 11;
  }
  // operator[] on missing key default-constructs
  if (m[7] != 0 || m.size() != 3) {
    return 12;
  }
  if (m.at(5) != 55) {
    return 13;
  }
  const std::unordered_map<int, int>& cm = m;
  if (cm.at(6) != 60) {
    return 14;
  }
  return 0;
}

int iteration_sum() {
  std::unordered_map<int, int> m = {{1, 1}, {2, 2}, {3, 3}, {4, 4}};
  if (m.size() != 4) {
    return 20;
  }
  int key_sum = 0;
  int val_sum = 0;
  for (std::unordered_map<int, int>::iterator it = m.begin(); it != m.end();
       ++it) {
    key_sum += it->first;
    val_sum += it->second;
  }
  if (key_sum != 10 || val_sum != 10) {
    return 21;
  }
  int range_sum = 0;
  for (const std::pair<const int, int>& kv : m) {
    range_sum += kv.second;
  }
  if (range_sum != 10) {
    return 22;
  }
  return 0;
}

int erase_variants() {
  std::unordered_map<int, int> m = {{1, 1}, {2, 2}, {3, 3}, {4, 4}};
  if (m.erase(2) != 1 || m.erase(99) != 0) {
    return 30;
  }
  if (m.size() != 3 || m.contains(2)) {
    return 31;
  }
  std::unordered_map<int, int>::iterator it = m.find(3);
  m.erase(it);
  if (m.contains(3) || m.size() != 2) {
    return 32;
  }
  m.clear();
  if (!m.empty()) {
    return 33;
  }
  return 0;
}

int emplace_try_insert_or_assign() {
  std::unordered_map<int, int> m;
  std::pair<std::unordered_map<int, int>::iterator, bool> e = m.emplace(1, 100);
  if (!e.second || e.first->second != 100) {
    return 40;
  }
  std::pair<std::unordered_map<int, int>::iterator, bool> e2 = m.emplace(1, 200);
  if (e2.second || e2.first->second != 100) {
    return 41;
  }
  std::pair<std::unordered_map<int, int>::iterator, bool> t = m.try_emplace(2, 20);
  if (!t.second || t.first->second != 20) {
    return 42;
  }
  std::pair<std::unordered_map<int, int>::iterator, bool> t2 = m.try_emplace(2, 999);
  if (t2.second || t2.first->second != 20) {
    return 43;
  }
  std::pair<std::unordered_map<int, int>::iterator, bool> a =
      m.insert_or_assign(2, 22);
  if (a.second || a.first->second != 22) {
    return 44;
  }
  std::pair<std::unordered_map<int, int>::iterator, bool> a2 =
      m.insert_or_assign(3, 33);
  if (!a2.second || a2.first->second != 33) {
    return 45;
  }
  return 0;
}

int string_keys() {
  std::unordered_map<std::string, int> m;
  m["alpha"] = 1;
  m["beta"] = 2;
  m["gamma"] = 3;
  if (m.size() != 3) {
    return 50;
  }
  if (m["beta"] != 2 || m.at("gamma") != 3) {
    return 51;
  }
  if (!m.contains("alpha") || m.contains("delta")) {
    return 52;
  }
  m["alpha"] += 10;
  if (m["alpha"] != 11) {
    return 53;
  }
  return 0;
}

int copy_and_move() {
  std::unordered_map<int, int> m = {{1, 10}, {2, 20}};
  std::unordered_map<int, int> copy = m;
  if (copy.size() != 2 || copy[1] != 10 || copy[2] != 20) {
    return 60;
  }
  copy[3] = 30;
  if (m.contains(3)) {
    return 61;  // deep copy: original unaffected
  }
  std::unordered_map<int, int> moved = std::move(copy);
  if (moved.size() != 3 || moved[3] != 30) {
    return 62;
  }
  return 0;
}

int rehash_growth() {
  std::unordered_map<int, int> m;
  for (int i = 0; i < 200; ++i) {
    m[i] = i * 2;
  }
  if (m.size() != 200) {
    return 70;
  }
  for (int i = 0; i < 200; ++i) {
    if (m[i] != i * 2) {
      return 71;
    }
  }
  int count = 0;
  for (const std::pair<const int, int>& kv : m) {
    (void)kv;
    count++;
  }
  if (count != 200) {
    return 72;
  }
  return 0;
}

int main(void) {
  int rc;
  if ((rc = basic_insert_lookup()) != 0) return rc;
  if ((rc = subscript_and_at()) != 0) return rc;
  if ((rc = iteration_sum()) != 0) return rc;
  if ((rc = erase_variants()) != 0) return rc;
  if ((rc = emplace_try_insert_or_assign()) != 0) return rc;
  if ((rc = string_keys()) != 0) return rc;
  if ((rc = copy_and_move()) != 0) return rc;
  if ((rc = rehash_growth()) != 0) return rc;
  return 0;
}
