// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <forward_list>
#include <list>
#include <vector>

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
};

int main() {
  std::vector<int> numbers;
  numbers.push_back(2);
  numbers.push_back(1);
  numbers.push_back(2);
  if (std::erase(numbers, numbers[0]) != 2 || numbers.size() != 1 ||
      numbers[0] != 1) {
    return 1;
  }

  std::list<Item> linked;
  linked.push_back(Item(2));
  linked.push_back(Item(1));
  linked.push_back(Item(2));
  if (std::erase(linked, linked.front()) != 2 || linked.size() != 1 ||
      !(linked.front() == Item(1))) {
    return 2;
  }

  std::forward_list<Item> forward;
  forward.push_front(Item(2));
  forward.push_front(Item(1));
  forward.push_front(Item(2));
  if (std::erase(forward, forward.front()) != 2 ||
      !(forward.front() == Item(1))) {
    return 3;
  }
  return 0;
}
