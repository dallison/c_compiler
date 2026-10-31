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

struct Boom {
  int value;
  static int copies_left;

  explicit Boom(int v) : value(v) {}
  Boom(const Boom& other) : value(other.value) {
    if (copies_left == 0) {
      throw 1;
    }
    --copies_left;
  }
  Boom(Boom&& other) : value(other.value) { other.value = -1; }
  Boom& operator=(const Boom& other) {
    value = other.value;
    return *this;
  }
  Boom& operator=(Boom&& other) {
    value = other.value;
    other.value = -1;
    return *this;
  }
};

int Boom::copies_left = 0;

int main() {
  std::vector<Item> values;
  values.push_back(Item(4));
  values.push_back(values[0]);
  if (values.size() != 2 || values[0].value != 4 || values[1].value != 4) {
    return 1;
  }
  values.push_back(std::move(values[0]));
  if (values.size() != 3 || values.back().value != 4 || values[1].value != 4) {
    return 2;
  }
  std::vector<Item> emplaced;
  emplaced.push_back(Item(5));
  emplaced.emplace_back(emplaced[0]);
  if (emplaced.size() != 2 || emplaced[0].value != 5 || emplaced[1].value != 5) {
    return 3;
  }

  std::vector<Item> spare;
  spare.reserve(4);
  spare.push_back(Item(1));
  spare.push_back(spare[0]);
  if (spare.size() != 2 || spare[0].value != 1 || spare[1].value != 1) {
    return 4;
  }

  std::vector<Item> grown;
  grown.push_back(Item(7));
  grown.resize(4, grown[0]);
  if (grown.size() != 4 || grown[0].value != 7 || grown[1].value != 7 ||
      grown[2].value != 7 || grown[3].value != 7) {
    return 5;
  }

  std::vector<Boom> items;
  items.push_back(Boom(4));
  Boom::copies_left = 0;
  try {
    items.push_back(items[0]);
    return 6;
  } catch (int) {
  }
  if (items.size() != 1 || items[0].value != 4) {
    return 7;
  }
  Boom::copies_left = 5;
  items.push_back(items[0]);
  if (items.size() != 2 || items[0].value != 4 || items[1].value != 4) {
    return 8;
  }
  return 0;
}
