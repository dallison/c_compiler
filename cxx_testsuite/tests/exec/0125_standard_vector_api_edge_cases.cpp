// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <vector>

int test_at_and_empty_state(void) {
  std::vector<int> values;
  if (!values.empty() || values.begin() != values.end() ||
      values.cbegin() != values.cend()) {
    return 1;
  }
  values.reserve(4);
  if (!values.empty() || values.begin() != values.end()) {
    return 2;
  }
  int threw = 0;
  try {
    (void)values.at(0);
  } catch (...) {
    threw = 1;
  }
  if (!threw || !values.empty()) {
    return 3;
  }

  values.push_back(11);
  values.push_back(22);
  if (values.at(0) != 11 || values.at(1) != 22) {
    return 4;
  }
  values.at(1) = 33;
  const std::vector<int>& cref = values;
  const int* ptr = cref.data();
  if (ptr[0] != 11 || cref.at(1) != 33) {
    return 5;
  }
  threw = 0;
  try {
    (void)cref.at(2);
  } catch (...) {
    threw = 1;
  }
  if (!threw || values.size() != 2 || values[1] != 33) {
    return 6;
  }
  return 0;
}

int test_modifier_edge_lifetimes(void) {
  std::vector<int> values;
  values.push_back(1);
  values.push_back(2);
  values.push_back(3);

  auto it = values.erase(values.begin(), values.end());
  if (it != values.begin() || !values.empty()) {
    return 10;
  }
  values.push_back(4);
  values.push_back(5);
  values.clear();
  if (!values.empty()) {
    return 11;
  }
  values.push_back(6);
  values.push_back(7);
  values.resize(5);
  if (values.size() != 5 || values[0] != 6 || values[2] != 0 ||
      values[4] != 0) {
    return 12;
  }
  values.resize(0);
  if (!values.empty() || values.begin() != values.end()) {
    return 13;
  }

  values.push_back(8);
  values.push_back(9);
  values.pop_back();
  if (values.size() != 1 || values.back() != 8) {
    return 14;
  }
  values.pop_back();
  if (!values.empty()) {
    return 15;
  }

  values.push_back(10);
  int* before_clear = values.data();
  values.clear();
  values.push_back(11);
  if (values.size() != 1 || values.front() != 11 ||
      values.data() != before_clear) {
    return 16;
  }
  return 0;
}

int main(void) {
  int result = test_at_and_empty_state();
  if (result != 0) {
    return result;
  }
  result = test_modifier_edge_lifetimes();
  if (result != 0) {
    return result;
  }
  return 0;
}
