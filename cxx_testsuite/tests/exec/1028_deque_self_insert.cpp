// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <deque>

struct Item {
  int value;

  explicit Item(int v) : value(v) {}
  Item(const Item& other) : value(other.value) {}
  Item(Item&& other) : value(other.value) { other.value = -1; }
  Item& operator=(const Item& other) {
    value = other.value;
    return *this;
  }
  Item& operator=(Item&& other) {
    value = other.value;
    other.value = -1;
    return *this;
  }
};

int main() {
  std::deque<Item> values;
  values.push_back(Item(1));
  values.push_back(Item(2));
  values.push_back(Item(3));
  values.insert(values.begin(), values.back());
  if (values.size() != 4 || values[0].value != 3 || values[1].value != 1 ||
      values[2].value != 2 || values[3].value != 3) {
    return 1;
  }
  values.insert(values.begin() + 1, 2, values.back());
  if (values.size() != 6 || values[0].value != 3 || values[1].value != 3 ||
      values[2].value != 3 || values[3].value != 1 || values[4].value != 2 ||
      values[5].value != 3) {
    return 2;
  }
  values.insert(values.end(), values[0]);
  if (values.size() != 7 || values.back().value != 3 || values[0].value != 3) {
    return 3;
  }
  return 0;
}
