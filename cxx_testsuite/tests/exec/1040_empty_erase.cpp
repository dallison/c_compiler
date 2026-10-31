// RUN: -std=c++26
// EXPECT_EXIT: 0

#include <deque>
#include <inplace_vector>

struct Item {
  int* value;
  explicit Item(int stored) : value(new int(stored)) {}
  Item(const Item&) = delete;
  Item& operator=(const Item&) = delete;
  Item(Item&& other) : value(other.value) { other.value = 0; }
  Item& operator=(Item&& other) {
    delete value;
    value = other.value;
    other.value = 0;
    return *this;
  }
  ~Item() { delete value; }
};

int main() {
  std::deque<Item> values;
  values.emplace_back(1);
  values.emplace_back(2);
  values.erase(values.begin(), values.begin());
  if (values.size() != 2) return 1;
  if (values[0].value == 0 || *values[0].value != 1) return 2;
  if (values[1].value == 0 || *values[1].value != 2) return 3;
  values.erase(values.begin(), values.begin() + 1);
  if (values.size() != 1 || values[0].value == 0 || *values[0].value != 2)
    return 4;

  std::inplace_vector<Item, 4> fixed;
  fixed.emplace_back(3);
  fixed.emplace_back(4);
  fixed.erase(fixed.begin(), fixed.begin());
  if (fixed.size() != 2) return 5;
  if (fixed[0].value == 0 || *fixed[0].value != 3) return 6;
  if (fixed[1].value == 0 || *fixed[1].value != 4) return 7;
  fixed.erase(fixed.begin(), fixed.begin() + 1);
  if (fixed.size() != 1 || fixed[0].value == 0 || *fixed[0].value != 4)
    return 8;
  return 0;
}
