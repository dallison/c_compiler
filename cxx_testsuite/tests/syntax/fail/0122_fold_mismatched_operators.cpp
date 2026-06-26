// RUN: -std=c++20
// EXPECT: fold expression operators must match

template <class... Ts>
int bad_fold_mismatched_operators(Ts... args) {
  return (0 + ... * args);
}
