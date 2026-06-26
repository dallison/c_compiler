// RUN: -std=c++20
// EXPECT: fold expression requires a non-pack initializer

template <class... Ts>
int bad_fold_pattern_seed(Ts... args) {
  return ((args + 1) + ... + (args + 2));
}
