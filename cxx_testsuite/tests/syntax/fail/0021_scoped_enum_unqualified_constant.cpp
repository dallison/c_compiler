// RUN: -std=c++17
// EXPECT: No such symbol "red"
enum class Color {
  red,
};

int main(void) {
  Color color = red;
  return sizeof(color);
}
