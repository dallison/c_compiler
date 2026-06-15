// RUN: -std=c++17
// EXPECT: const_cast requires pointer or reference to the same type
int main(void) {
  int value = 7;
  int other = const_cast<int>(value);
  return other;
}
