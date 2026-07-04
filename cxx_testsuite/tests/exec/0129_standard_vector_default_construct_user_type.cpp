// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <vector>

int live_defaults = 0;

struct DefaultValue {
  int value;

  DefaultValue() : value(42) {
    ++live_defaults;
  }
  DefaultValue(const DefaultValue& other) : value(other.value) {
    ++live_defaults;
  }
  DefaultValue(DefaultValue&& other) : value(other.value) {
    other.value = -1;
    ++live_defaults;
  }
  DefaultValue& operator=(const DefaultValue& other) {
    value = other.value;
    return *this;
  }
  DefaultValue& operator=(DefaultValue&& other) {
    value = other.value;
    other.value = -1;
    return *this;
  }
  ~DefaultValue() {
    --live_defaults;
  }
};

int main(void) {
  {
    std::vector<DefaultValue> values;
    values.resize(3);
    if (values.size() != 3 || values[0].value != 42 ||
        values[1].value != 42 || values[2].value != 42) {
      return 1;
    }
    values.resize(1);
    if (values.size() != 1 || values[0].value != 42) {
      return 2;
    }
    values.clear();
    if (!values.empty() || live_defaults != 0) {
      return 3;
    }
    values.resize(2);
    if (values.size() != 2 || values[0].value != 42 ||
        values[1].value != 42) {
      return 4;
    }
  }
  if (live_defaults != 0) {
    return 5;
  }
  return 0;
}
