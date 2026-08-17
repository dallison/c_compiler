// RUN: -std=c++26
// EXPECT: 'main' cannot be declared constexpr

constexpr int main() {
  return 0;
}
