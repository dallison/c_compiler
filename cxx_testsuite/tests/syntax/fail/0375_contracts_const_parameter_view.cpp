// RUN: -std=c++26
// EXPECT: Cannot increment or decrement this value

int invalid_mutation(int value)
    pre (++value > 0) {
  return value;
}
