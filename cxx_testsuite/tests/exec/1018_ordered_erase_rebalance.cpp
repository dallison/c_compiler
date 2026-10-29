// RUN: -std=c++20
// EXPECT_EXIT: 0

// Erase has to repair red-black colors.  A later insert rotates using those
// colors; a broken tree no longer iterates in order.

#include <map>
#include <set>

int check_map() {
  std::map<int, int> values;
  for (int i = 0; i < 64; ++i) {
    values[i] = i;
  }
  for (int i = 0; i < 64; i += 2) {
    if (values.erase(i) != 1) {
      return 1;
    }
  }
  for (int i = 64; i < 96; ++i) {
    values[i] = i;
  }
  int prev = -1;
  int count = 0;
  for (std::map<int, int>::iterator it = values.begin(); it != values.end();
       ++it) {
    if (it->first <= prev || it->second != it->first) {
      return 2;
    }
    if (values.find(it->first)->second != it->first) {
      return 3;
    }
    prev = it->first;
    ++count;
  }
  if (count != 64) {
    return 4;
  }
  while (!values.empty()) {
    values.erase(values.begin());
  }
  if (values.size() != 0 || values.begin() != values.end()) {
    return 5;
  }
  return 0;
}

int check_set() {
  std::set<int> values;
  for (int i = 63; i >= 0; --i) {
    values.insert(i);
  }
  for (int i = 1; i < 64; i += 2) {
    values.erase(values.find(i));
  }
  for (int i = 100; i < 116; ++i) {
    values.insert(i);
  }
  int prev = -1;
  int count = 0;
  for (std::set<int>::iterator it = values.begin(); it != values.end(); ++it) {
    if (*it <= prev || values.find(*it) == values.end()) {
      return 6;
    }
    prev = *it;
    ++count;
  }
  if (count != 48 || values.count(1) != 0 || values.count(0) != 1 ||
      values.count(100) != 1) {
    return 7;
  }
  return 0;
}

int check_multimap() {
  std::multimap<int, int> values;
  for (int i = 0; i < 40; ++i) {
    values.insert(std::pair<const int, int>(i % 8, i));
  }
  std::multimap<int, int>::iterator it = values.begin();
  while (it != values.end()) {
    if ((it->second & 1) == 0) {
      it = values.erase(it);
    } else {
      ++it;
    }
  }
  for (int i = 0; i < 8; ++i) {
    values.insert(std::pair<const int, int>(i, 100 + i));
  }
  int prev = -1;
  int count = 0;
  for (it = values.begin(); it != values.end(); ++it) {
    if (it->first < prev) {
      return 8;
    }
    prev = it->first;
    ++count;
  }
  if (count != (int)values.size() || values.size() != 28) {
    return 9;
  }
  return 0;
}

int main() {
  int rc = check_map();
  if (rc != 0) {
    return rc;
  }
  rc = check_set();
  if (rc != 0) {
    return rc;
  }
  return check_multimap();
}
