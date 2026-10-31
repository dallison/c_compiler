// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <map>
#include <set>
#include <unordered_set>

struct Item {
  int* value;

  explicit Item(int held) : value(new int(held)) {}
  Item(const Item& other) : value(new int(*other.value)) {}
  Item& operator=(const Item& other) {
    if (this != &other) *value = *other.value;
    return *this;
  }
  ~Item() {
    delete value;
    value = 0;
  }
  bool operator==(const Item& other) const { return *value == *other.value; }
  bool operator<(const Item& other) const { return *value < *other.value; }
};

namespace std {
template <>
struct hash<Item> {
  size_t operator()(const Item& item) const { return hash<int>()(*item.value); }
};
}

int main() {
  std::multiset<Item> ordered;
  std::multiset<Item>::iterator ordered_one = ordered.insert(Item(1));
  ordered.insert(Item(1));
  ordered.insert(Item(2));
  if (ordered.erase(*ordered_one) != 2 || ordered.size() != 1 ||
      !(*ordered.begin() == Item(2))) {
    return 1;
  }

  std::multimap<Item, int> mapped;
  std::multimap<Item, int>::iterator mapped_one =
      mapped.insert(std::pair<Item, int>(Item(1), 10));
  mapped.insert(std::pair<Item, int>(Item(1), 11));
  mapped.insert(std::pair<Item, int>(Item(2), 12));
  if (mapped.erase(mapped_one->first) != 2 || mapped.size() != 1 ||
      !(mapped.begin()->first == Item(2))) {
    return 2;
  }

  std::unordered_multiset<Item> hashed;
  std::unordered_multiset<Item>::iterator hashed_one = hashed.insert(Item(1));
  hashed.insert(Item(1));
  hashed.insert(Item(2));
  if (hashed.erase(*hashed_one) != 2 || hashed.size() != 1 ||
      !(*hashed.begin() == Item(2))) {
    return 3;
  }
  return 0;
}
