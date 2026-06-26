// RUN: -std=c++20
// EXPECT: fold expression requires a parameter pack

template <class... Ts>
int bad_fold_requires_pack(int value, Ts... args) {
  (void)args;
  return (value + ... + 0);
}
