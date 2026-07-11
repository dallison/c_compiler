// RUN: -std=c++20
// EXPECT_EXIT: 0
#include <functional>
#include <memory>
#include <utility>

struct Tracker {
  static int live;
  int value;

  Tracker() : value(0) { live++; }
  Tracker(int v) : value(v) { live++; }
  Tracker(const Tracker& other) : value(other.value) { live++; }
  Tracker(Tracker&& other) : value(other.value) {
    other.value = -1;
    live++;
  }
  Tracker& operator=(const Tracker& other) {
    value = other.value;
    return *this;
  }
  Tracker& operator=(Tracker&& other) {
    value = other.value;
    other.value = -1;
    return *this;
  }
  ~Tracker() { live--; }
};

int Tracker::live = 0;

int pair_forwarding() {
  {
    std::pair<int, Tracker> value(7, Tracker(9));
    if (value.first != 7 || value.second.value != 9) {
      return 1;
    }
    std::pair<int, int> made(3, 4);
    if (made.first != 3 || made.second != 4) {
      return 3;
    }
    if (!(std::pair<int, int>(1, 2) == std::pair<int, int>(1, 2))) {
      return 4;
    }
    if (!((std::pair<int, int>(1, 2) <=> std::pair<int, int>(1, 3)) < 0)) {
      return 5;
    }
  }
  return Tracker::live;
}

int allocator_rebind() {
  using Alloc = std::allocator<std::pair<const int, int> >;
  using NodeAlloc =
      typename std::allocator_traits<Alloc>::template rebind_alloc<Tracker>;
  (void)sizeof(NodeAlloc);
  std::allocator<Tracker> alloc;
  Tracker* ptr = std::allocator_traits<std::allocator<Tracker> >::allocate(alloc, 1);
  std::allocator_traits<std::allocator<Tracker> >::construct(alloc, ptr);
  ptr->value = 42;
  if (ptr->value != 42 || Tracker::live != 1) {
    return 10;
  }
  std::allocator_traits<std::allocator<Tracker> >::destroy(alloc, ptr);
  std::allocator_traits<std::allocator<Tracker> >::deallocate(alloc, ptr, 1);
  if (Tracker::live != 0) {
    return 20;
  }
  return 0;
}

int comparators() {
  std::less<void> less;
  std::greater<int> greater;
  std::equal_to<void> equal;
  if (!less(1, 2) || less(2, 1)) {
    return 1;
  }
  if (!greater(4, 3) || greater(3, 4)) {
    return 2;
  }
  if (!equal(5, 5) || equal(5, 6)) {
    return 3;
  }
  return 0;
}

int main() {
  if (pair_forwarding() != 0) {
    return 1;
  }
  if (allocator_rebind() != 0) {
    return 2;
  }
  if (comparators() != 0) {
    return 3;
  }
  return 0;
}
