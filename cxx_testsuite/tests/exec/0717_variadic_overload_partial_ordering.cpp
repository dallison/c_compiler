// RUN: -std=c++17
// EXPECT_EXIT: 0

template <class Key, class Value>
struct Slot {};

struct Policy {
  template <class Allocator, class... Args>
  static int construct(Allocator*, Slot<int, int>*, Args&&...) {
    return 1;
  }

  template <class Allocator>
  static int construct(Allocator*, Slot<int, int>*, Slot<int, int>*) {
    return 2;
  }

  template <class Allocator>
  static int construct(Allocator*, Slot<int, int>*,
                       const Slot<int, int>*) {
    return 3;
  }
};

int main() {
  int allocator = 0;
  Slot<int, int> destination;
  Slot<int, int> source;
  return Policy::construct(&allocator, &destination, &source) == 2 ? 0 : 1;
}
