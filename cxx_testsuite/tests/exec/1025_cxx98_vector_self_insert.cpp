// RUN: -std=c++98
// EXPECT_EXIT: 0

#include <vector>

int copies_left = 0;

struct Item {
  int value;
  explicit Item(int v) : value(v) {}
  Item(const Item& other) : value(other.value) {
    if (copies_left == 0) throw 1;
    --copies_left;
  }
};

int main() {
  std::vector<int> grown;
  grown.push_back(4);
  grown.push_back(grown[0]);
  if (grown.size() != 2 || grown[0] != 4 || grown[1] != 4) return 1;
  grown.push_back(grown[0]);
  if (grown.size() != 3 || grown[0] != 4 || grown[1] != 4 || grown[2] != 4) {
    return 2;
  }

  std::vector<int> spare;
  spare.push_back(1);
  spare.push_back(2);
  spare.push_back(3);
  int* first = &spare[0];
  spare.insert(spare.begin() + 1, spare.back());
  if (spare.size() != 4 || spare[0] != 1 || spare[1] != 3 || spare[2] != 2 ||
      spare[3] != 3 || first != &spare[0] || *first != 1) {
    return 3;
  }
  spare.insert(spare.begin(), spare[3]);
  if (spare.size() != 5 || spare[0] != 3 || spare[1] != 1 || spare[2] != 3 ||
      spare[3] != 2 || spare[4] != 3) {
    return 4;
  }

  std::vector<int> shifted;
  shifted.push_back(1);
  shifted.push_back(2);
  shifted.insert(shifted.begin(), shifted[1]);
  if (shifted.size() != 3 || shifted[0] != 2 || shifted[1] != 1 ||
      shifted[2] != 2) {
    return 5;
  }

  std::vector<Item> items;
  copies_left = 5;
  items.push_back(Item(7));
  if (items.size() != 1 || items.capacity() != 1 || items[0].value != 7) {
    return 6;
  }
  copies_left = 1;
  try {
    items.push_back(items[0]);
    return 7;
  } catch (int) {
  }
  if (items.size() != 1 || items.capacity() != 1 || items[0].value != 7) {
    return 8;
  }
  return 0;
}
