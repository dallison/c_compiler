// RUN: -std=c++20
// EXPECT: fold expression requires an operator

template <class... Values>
auto invalid_spaceship_fold(Values... values) {
  return (... <=> values);
}

int use_invalid_spaceship_fold() { return invalid_spaceship_fold(1, 2); }
