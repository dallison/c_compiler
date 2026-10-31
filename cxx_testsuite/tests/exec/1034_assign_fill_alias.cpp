// RUN: -std=c++20
// EXPECT_EXIT: 0

#include <deque>
#include <forward_list>
#include <list>

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

template <class Container>
int check_assign(Container& values) {
  values.assign(3, values.front());
  if (values.front() == Item(4)) return 0;
  return 1;
}

int main() {
  std::deque<Item> deck;
  deck.push_back(Item(4));
  if (check_assign(deck) != 0) return 1;
  if (deck.size() != 3) return 2;

  std::list<Item> linked;
  linked.push_back(Item(4));
  if (check_assign(linked) != 0) return 3;
  if (linked.size() != 3) return 4;

  std::forward_list<Item> forward;
  forward.push_front(Item(4));
  forward.assign(3, forward.front());
  if (!(forward.front() == Item(4))) return 5;
  int count = 0;
  for (typename std::forward_list<Item>::iterator it = forward.begin();
       it != forward.end(); ++it) {
    if (!(*it == Item(4))) return 6;
    ++count;
  }
  if (count != 3) return 7;
  return 0;
}
