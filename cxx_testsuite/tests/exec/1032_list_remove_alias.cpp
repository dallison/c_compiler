// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <forward_list>
#include <list>

struct Item {
  int* value;

  explicit Item(int held) : value(new int(held)) {}
  Item(const Item& other) : value(new int(*other.value)) {}
  Item& operator=(const Item& other) {
    if (this != &other) {
      *value = *other.value;
    }
    return *this;
  }
  ~Item() {
    delete value;
    value = 0;
  }
  bool operator==(const Item& other) const { return *value == *other.value; }
};

int main() {
  std::list<Item> values;
  values.push_back(Item(1));
  values.push_back(Item(2));
  values.push_back(Item(1));
  if (values.remove(values.front()) != 2) return 1;
  if (values.size() != 1 || !(values.front() == Item(2))) return 2;

  std::forward_list<Item> forward;
  forward.push_front(Item(1));
  forward.push_front(Item(2));
  forward.push_front(Item(1));
  if (forward.remove(forward.front()) != 2) return 3;
  if (!(forward.front() == Item(2))) return 4;

  std::list<int> chain;
  chain.push_back(1);
  chain.push_back(2);
  chain.splice(chain.begin(), chain);
  if (chain.size() != 2 || chain.front() != 1 || *(++chain.begin()) != 2) {
    return 5;
  }
  return 0;
}
