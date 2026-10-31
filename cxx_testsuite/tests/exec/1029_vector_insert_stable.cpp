// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <utility>
#include <vector>

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
  std::vector<Item> values;
  values.reserve(8);
  values.push_back(Item(1));
  values.push_back(Item(2));
  values.push_back(Item(3));
  Item* first = &values[0];
  values.insert(values.begin() + 1, values.back());
  if (first != &values[0] || first->value != 1) return 1;
  if (values.size() != 4 || values[1].value != 3 || values[2].value != 2 ||
      values[3].value != 3) {
    return 2;
  }
  values.insert(values.end(), values[0]);
  if (first != &values[0] || values.back().value != 1 || values[0].value != 1) {
    return 3;
  }
  values.insert(values.begin() + 2, 2, values.back());
  if (first != &values[0] || values[0].value != 1 || values[2].value != 1 ||
      values[3].value != 1) {
    return 4;
  }

  std::vector<Item> full;
  full.push_back(Item(4));
  full.push_back(Item(5));
  full.insert(full.begin(), full[1]);
  if (full.size() != 3 || full[0].value != 5 || full[1].value != 4 ||
      full[2].value != 5) {
    return 5;
  }
  return 0;
}
