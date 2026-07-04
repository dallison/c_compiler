// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <vector>

struct AliasTracked {
  int value;
  bool safe_source;

  static int live;
  static int destroyed;

  AliasTracked() : value(0), safe_source(false) {
    ++live;
  }

  explicit AliasTracked(int v) : value(v), safe_source(false) {
    ++live;
  }

  AliasTracked(const AliasTracked& other) : safe_source(true) {
    if (other.safe_source || destroyed == 0) {
      value = other.value;
    } else {
      value = -1000;
    }
    ++live;
  }

  AliasTracked(AliasTracked&& other) : value(other.value), safe_source(true) {
    other.value = -1;
    ++live;
  }

  AliasTracked& operator=(const AliasTracked& other) {
    value = other.value;
    safe_source = true;
    return *this;
  }

  AliasTracked& operator=(AliasTracked&& other) {
    value = other.value;
    safe_source = true;
    other.value = -1;
    return *this;
  }

  ~AliasTracked() {
    --live;
    ++destroyed;
  }
};

int AliasTracked::live = 0;
int AliasTracked::destroyed = 0;

int test_assign_fill_alias(void) {
  AliasTracked::destroyed = 0;
  std::vector<AliasTracked> values;
  values.emplace_back(11);
  values.emplace_back(22);
  values.emplace_back(33);
  values.assign(4, values[1]);
  if (values.size() != 4) {
    return 1;
  }
  for (unsigned long i = 0; i < values.size(); i++) {
    if (values[i].value != 22) {
      return 2;
    }
  }
  values.clear();
  return 0;
}

int test_assign_range_self_alias(void) {
  std::vector<int> values;
  values.push_back(1);
  values.push_back(2);
  values.push_back(3);
  values.push_back(4);
  values.assign(values.begin() + 1, values.end() - 1);
  if (values.size() != 2) {
    return 10;
  }
  if (values[0] != 2 || values[1] != 3) {
    return 11;
  }
  values.assign(values.begin(), values.end());
  if (values.size() != 2) {
    return 12;
  }
  if (values[0] != 2 || values[1] != 3) {
    return 13;
  }
  return 0;
}

int main(void) {
  int result = test_assign_fill_alias();
  if (result != 0) {
    return result;
  }
  result = test_assign_range_self_alias();
  if (result != 0) {
    return result;
  }
  return 0;
}
