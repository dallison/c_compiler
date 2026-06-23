// RUN: -std=c++20
// EXPECT: Class template argument deduction requires an initializer

template <typename T>
struct Holder {
  T value;
  Holder(T initial) : value(initial) {
  }
};

void fail_new_ctad_requires_initializer() {
  auto value = new Holder;
  (void)value;
}
