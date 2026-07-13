// RUN: -std=c++20
#include <concepts>
#include <functional>

struct Ordered {
  int value;
  // std::regular requires default_initializable, so provide a default
  // constructor in addition to the value constructor.
  Ordered() : value(0) {}
  Ordered(int v) : value(v) {}
  bool operator==(const Ordered& other) const { return value == other.value; }
  bool operator!=(const Ordered& other) const { return !(*this == other); }
  bool operator<(const Ordered& other) const { return value < other.value; }
  bool operator>(const Ordered& other) const { return value > other.value; }
  bool operator<=(const Ordered& other) const { return value <= other.value; }
  bool operator>=(const Ordered& other) const { return value >= other.value; }
};

struct AdlSwap {
  int value;
  AdlSwap(int v) : value(v) {}
};

void swap(AdlSwap& left, AdlSwap& right) {
  int tmp = left.value;
  left.value = right.value;
  right.value = tmp;
}

int main() {
  if (!std::same_as<int, int>) {
    return 1;
  }
  if (!std::integral<int>) {
    return 2;
  }
  if (!std::movable<Ordered>) {
    return 3;
  }
  if (!std::regular<Ordered>) {
    return 4;
  }
  if (!std::totally_ordered<Ordered>) {
    return 5;
  }
  if (!(Ordered(1) < Ordered(3))) {
    return 6;
  }
  AdlSwap left(1);
  AdlSwap right(2);
  if (!std::swappable_with<AdlSwap&, AdlSwap&>) {
    return 7;
  }
  std::ranges::swap(left, right);
  if (left.value != 2 || right.value != 1) {
    return 8;
  }
  auto is_positive = [](int value) { return value > 0; };
  if (!is_positive(3)) {
    return 9;
  }
  if (!std::invocable<decltype(is_positive), int>) {
    return 10;
  }
  return 0;
}
