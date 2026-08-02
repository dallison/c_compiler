// RUN: -std=c++20
// EXPECT: Wrong number of args for builtin; expected 2, got 1

int main(void) {
  return __builtin_expect(1);
}
