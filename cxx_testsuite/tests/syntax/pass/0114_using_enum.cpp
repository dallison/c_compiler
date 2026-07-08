// RUN: -std=c++20
// C++20 `using enum`: bring an enumeration's enumerators into the current scope.
enum class Color { Red, Green, Blue };

namespace pal {
enum class Shade { Light, Dark };
}

// Namespace/global-scope using-enum with a qualified name.
using enum pal::Shade;

int local_scope() {
  using enum Color;
  Color c = Green;
  return (c == Red) ? 1 : 0;
}

int switch_labels(Color c) {
  using enum Color;
  switch (c) {
    case Red:
      return 0;
    case Green:
      return 1;
    case Blue:
      return 2;
  }
  return -1;
}

// Unscoped enum re-introduced in a nested scope.
enum Fruit { Apple, Pear };
int unscoped() {
  using enum Fruit;
  return (Apple == 0 && Pear == 1) ? 0 : 1;
}

int main(void) {
  return local_scope() + switch_labels(Color::Red) +
         (Dark == pal::Shade::Dark ? 0 : 1) + unscoped();
}
