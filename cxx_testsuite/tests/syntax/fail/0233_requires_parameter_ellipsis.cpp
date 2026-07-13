// RUN: -std=c++20
// EXPECT: requires-expression parameter list cannot end with an ellipsis
template <typename T>
concept EllipsisParam = requires (int x, ...) { x; };

int main(void) {
  return 0;
}
