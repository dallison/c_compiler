// RUN: -std=c++20
// EXPECT: Conditional operator with void expression requires both arms to be void

void side_effect(void);

int value(bool cond) {
  return cond ? 1 : side_effect();
}
