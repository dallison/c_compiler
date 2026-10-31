// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <forward_list>

struct Item {
  int n;
  static int budget;

  bool operator<(const Item& other) const {
    if (budget == 0) {
      throw 1;
    }
    --budget;
    return n < other.n;
  }
};

int Item::budget = 0;

int main() {
  std::forward_list<Item> values;
  for (int i = 0; i < 12; ++i) {
    Item item;
    item.n = i + 1;
    values.push_front(item);
  }
  Item::budget = 0;
  int threw = 0;
  try {
    values.sort();
  } catch (int) {
    threw = 1;
  }
  if (!threw) return 1;
  int seen[12];
  for (int i = 0; i < 12; ++i) seen[i] = 0;
  int count = 0;
  for (std::forward_list<Item>::iterator it = values.begin();
       it != values.end(); ++it) {
    if (it->n < 1 || it->n > 12 || seen[it->n - 1] != 0) return 2;
    seen[it->n - 1] = 1;
    ++count;
  }
  if (count != 12) return 3;

  Item::budget = 1000;
  values.sort();
  int expect = 1;
  for (std::forward_list<Item>::iterator it = values.begin();
       it != values.end(); ++it) {
    if (it->n != expect) return 4;
    ++expect;
  }
  return expect == 13 ? 0 : 5;
}
