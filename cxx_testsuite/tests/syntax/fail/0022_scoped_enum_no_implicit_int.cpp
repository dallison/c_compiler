// RUN: -std=c++17
// EXPECT: Illegal conversion; cannot convert from
enum class Color {
  red,
};

int main(void) {
  int value = Color::red;
  return value;
}
