// RUN: -std=c++20
// EXPECT_EXIT: 0

enum class Color {
  red = 3,
  green = 5,
  blue = 7,
};

struct Palette {
  using enum Color;

  Color selected = red;

  static int blue_value() {
    return static_cast<int>(blue);
  }
};

template <typename T>
struct Box {
  using enum Color;

  T adjustment;
  Color selected = green;

  int value() const {
    return static_cast<int>(selected) + adjustment;
  }
};

int main() {
  Palette palette;
  if (palette.selected != Color::red || Palette::blue != Color::blue ||
      Palette::blue_value() != 7) {
    return 1;
  }

  Box<int> box{4};
  if (box.selected != Color::green || Box<int>::red != Color::red ||
      box.value() != 9) {
    return 2;
  }
  return 0;
}
