// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <compare>
#include <initializer_list>
#include <vector>

int test_access_and_iterators(void) {
  std::vector<int> values;
  if (!values.empty() || values.begin() != values.end() ||
      values.cbegin() != values.cend()) {
    return 1;
  }

  values.push_back(1);
  values.push_back(2);
  values.push_back(3);
  if (values.empty() || values.front() != 1 || values.back() != 3) {
    return 2;
  }
  values.front() = 10;
  values.back() = 30;
  if (values.data()[0] != 10 || values[2] != 30) {
    return 3;
  }

  const std::vector<int>& cref = values;
  if (*cref.begin() != 10 || *(cref.end() - 1) != 30 ||
      *cref.cbegin() != 10 || *(cref.cend() - 1) != 30) {
    return 4;
  }
  if (*values.rbegin() != 30 || *cref.rbegin() != 30 ||
      *cref.crbegin() != 30) {
    return 5;
  }
  *values.rbegin() = 40;
  if (values.back() != 40) {
    return 6;
  }
  auto rit = values.rbegin();
  ++rit;
  if (*rit != 2 || *(cref.crend() - 1) != 10) {
    return 7;
  }
  return 0;
}

int test_modifiers(void) {
  std::vector<int> values;
  values.push_back(1);
  values.push_back(2);
  values.push_back(3);
  values.push_back(4);

  std::vector<int>::iterator unchanged =
      values.erase(values.begin() + 1, values.begin() + 1);
  if (unchanged != values.begin() + 1 || values.size() != 4 ||
      values[1] != 2) {
    return 10;
  }

  std::vector<int>::iterator next = values.erase(values.begin() + 1);
  if (next != values.begin() + 1 || values.size() != 3 || *next != 3) {
    return 11;
  }

  next = values.erase(values.end() - 1);
  if (next != values.end() || values.size() != 2 || values.back() != 3) {
    return 12;
  }

  std::initializer_list<int> extra = {7, 8, 9};
  next = values.insert(values.begin() + 1, extra);
  if (next != values.begin() + 1 || values.size() != 5 || values[0] != 1 ||
      values[1] != 7 || values[3] != 9 || values[4] != 3) {
    return 13;
  }

  next = values.erase(values.begin() + 1, values.begin() + 4);
  if (next != values.begin() + 1 || values.size() != 2 || values[0] != 1 ||
      values[1] != 3) {
    return 14;
  }

  values.pop_back();
  if (values.size() != 1 || values.back() != 1) {
    return 15;
  }
  values.pop_back();
  if (!values.empty() || values.begin() != values.end()) {
    return 16;
  }
  return 0;
}

int test_comparisons(void) {
  std::vector<int> a;
  a.push_back(1);
  a.push_back(2);
  std::vector<int> b;
  b.push_back(1);
  b.push_back(2);
  std::vector<int> c;
  c.push_back(1);
  c.push_back(3);
  std::vector<int> d;
  d.push_back(1);
  d.push_back(2);
  d.push_back(0);

  if (!(a == b) || a != b || !(a != c)) {
    return 20;
  }
  if (!(a < c) || !(c > a) || !(a <= b) || !(a >= b)) {
    return 21;
  }
  if (!(a < d) || !(d > a)) {
    return 22;
  }
  std::strong_ordering equal = (a <=> b);
  std::strong_ordering less = (a <=> c);
  std::strong_ordering greater = (c <=> a);
  if (!(equal == 0) || !(less < 0) || !(greater > 0)) {
    return 23;
  }
  return 0;
}

int main(void) {
  int result = test_access_and_iterators();
  if (result != 0) {
    return result;
  }
  result = test_modifiers();
  if (result != 0) {
    return result;
  }
  result = test_comparisons();
  if (result != 0) {
    return result;
  }
  return 0;
}
