// RUN: -std=c++20
// EXPECT: requires-expression parameters cannot have default arguments
template <typename T>
concept DefaultParam = requires (T value = T()) { value; };

int main(void) {
  return 0;
}
