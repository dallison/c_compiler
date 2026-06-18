// RUN: -std=c++20
int main(void) {
  auto no_args = [] {
    return 3;
  };
  auto add = [](int a, int b) {
    return a + b;
  };
  int x = no_args();
  int y = add(4, 5);
  int base = 10;
  auto value_capture = [base](int extra) {
    return base + extra;
  };
  auto ref_capture = [&base] {
    base = base + 1;
    return base;
  };
  auto default_value = [=] {
    return base + x;
  };
  auto default_ref = [&] {
    y = y + 1;
    return y;
  };
  auto mutable_value = [base]() mutable {
    base = base + 3;
    return base;
  };
  auto trailing = [](int n) -> int {
    return n + 1;
  };
  auto specifiers = [](int n) constexpr noexcept -> int {
    return n;
  };
  auto generic = [](auto n) {
    return n + 4;
  };
  int values[3] = {1, 2, 3};
  int sum = 0;
  auto add_to_sum = [&](int n) {
    sum = sum + n;
    return sum;
  };
  for (auto value : values) {
    add_to_sum(value);
  }
  return x + y + value_capture(2) + ref_capture() + default_value() +
         default_ref() + mutable_value() + trailing(1) + specifiers(2) +
         generic(3) + sum;
}
