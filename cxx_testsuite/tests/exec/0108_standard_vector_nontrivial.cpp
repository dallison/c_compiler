// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <vector>

struct Tracked {
  int value;

  static int live;
  static int constructed;
  static int destroyed;
  static int copied;
  static int moved;
  static int copy_assigned;
  static int move_assigned;

  Tracked() : value(0) {
    ++live;
    ++constructed;
  }

  explicit Tracked(int v) : value(v) {
    ++live;
    ++constructed;
  }

  Tracked(const Tracked& other) : value(other.value) {
    ++live;
    ++constructed;
    ++copied;
  }

  Tracked(Tracked&& other) : value(other.value) {
    other.value = -1;
    ++live;
    ++constructed;
    ++moved;
  }

  Tracked& operator=(const Tracked& other) {
    value = other.value;
    ++copy_assigned;
    return *this;
  }

  Tracked& operator=(Tracked&& other) {
    value = other.value;
    other.value = -1;
    ++move_assigned;
    return *this;
  }

  ~Tracked() {
    --live;
    ++destroyed;
  }
};

int Tracked::live = 0;
int Tracked::constructed = 0;
int Tracked::destroyed = 0;
int Tracked::copied = 0;
int Tracked::moved = 0;
int Tracked::copy_assigned = 0;
int Tracked::move_assigned = 0;

int main(void) {
  {
    std::vector<Tracked> items;
    items.reserve(2);
    items.emplace_back(1);
    Tracked two(2);
    items.push_back(two);
    items.push_back(Tracked(3));
    if (items.size() != 3 || items[0].value != 1 || items[1].value != 2 ||
        items[2].value != 3) {
      return 1;
    }

    items.insert(items.begin() + 1, Tracked(7));
    if (items.size() != 4) {
      return 2;
    }
    if (items[1].value != 7) {
      return 10;
    }
    if (items[2].value != 2) {
      return 11;
    }

    items.erase(items.begin() + 1);
    if (items.size() != 3 || items[1].value != 2 || items[2].value != 3) {
      return 3;
    }

    items.resize(5, Tracked(9));
    if (items.size() != 5 || items[3].value != 9 || items[4].value != 9) {
      return 4;
    }

    items.pop_back();
    if (items.size() != 4) {
      return 5;
    }

    items.clear();
    if (!items.empty()) {
      return 6;
    }
  }

  if (Tracked::live != 0) {
    return 7;
  }
  if (Tracked::constructed != Tracked::destroyed) {
    return 8;
  }
  if (Tracked::copied == 0 || Tracked::moved == 0 ||
      Tracked::move_assigned == 0) {
    return 9;
  }
  return 0;
}
