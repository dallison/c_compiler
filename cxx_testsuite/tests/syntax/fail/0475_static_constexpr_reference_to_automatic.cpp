// RUN: -std=c++26

void invalid_static_reference() {
  int value = 42;
  static constexpr int& reference = value;
}

constexpr const int& pass_through(const int& value) {
  return value;
}

void invalid_cross_scope_reference() {
  constexpr const int& reference = pass_through(42);
}
