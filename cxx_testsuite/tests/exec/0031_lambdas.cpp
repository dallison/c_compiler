int main(void) {
  auto no_args = [] {
    return 3;
  };
  auto add = [](int a, int b) {
    return a + b;
  };
  if (no_args() != 3) {
    return 1;
  }
  if (add(4, 5) != 9) {
    return 2;
  }
  if ([] { return 7; }() != 7) {
    return 3;
  }
  int base = 10;
  if ([base](int extra) { return base + extra; }(2) != 12) {
    return 8;
  }
  auto value_capture = [base](int extra) {
    return base + extra;
  };
  if (value_capture(2) != 12) {
    return 4;
  }
  auto ref_capture = [&base] {
    base = base + 1;
    return base;
  };
  if (ref_capture() != 11 || base != 11) {
    return 5;
  }
  int x = 3;
  auto default_value = [=] {
    return base + x;
  };
  if (default_value() != 14) {
    return 6;
  }
  auto default_ref = [&] {
    x = x + 4;
    return x;
  };
  if (default_ref() != 7 || x != 7) {
    return 7;
  }
  auto mutable_value = [base]() mutable {
    base = base + 3;
    return base;
  };
  if (mutable_value() != 14 || base != 11) {
    return 9;
  }
  auto trailing = [](int n) -> int {
    return n + 1;
  };
  if (trailing(2) != 3) {
    return 10;
  }
  auto specifiers = [](int n) constexpr noexcept -> int {
    return n;
  };
  if (specifiers(5) != 5) {
    return 11;
  }
  auto generic = [](auto n) {
    return n + 4;
  };
  if (generic(3) != 7) {
    return 12;
  }
  int values[3] = {1, 2, 3};
  int sum = 0;
  auto add_to_sum = [&](int n) {
    sum = sum + n;
    return sum;
  };
  for (auto value : values) {
    add_to_sum(value);
  }
  if (sum != 6) {
    return 13;
  }
  return 0;
}
