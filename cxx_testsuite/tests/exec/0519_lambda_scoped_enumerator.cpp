// RUN: -std=c++20
// EXPECT_EXIT: 0

enum class Color { red = 3, green = 5 };
enum Plain { blue = 7 };

int main() {
  int offset = 1;
  auto fn = [&] {
    return static_cast<int>(Color::red) + offset;
  };
  if (fn() != 4) {
    return 1;
  }
  auto by_value = [=] {
    return static_cast<int>(Color::green);
  };
  if (by_value() != 5) {
    return 2;
  }
  auto unscoped = [&] { return blue; };
  if (unscoped() != 7) {
    return 3;
  }
  return 0;
}
