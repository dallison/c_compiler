// RUN: -std=c++17
enum class Color {
  red,
  green = 4,
};

enum struct Shape {
  circle,
  square,
};

namespace paint {
enum class Finish {
  matte,
  gloss,
};
}

int main(void) {
  Color color = Color::red;
  Color other = Color::green;
  Shape shape = Shape::circle;
  paint::Finish finish = paint::Finish::gloss;
  return sizeof(color) + sizeof(other) + sizeof(shape) + sizeof(finish);
}
