// RUN: -std=c++23 -fconstexpr-eval=pcode

struct non_literal {
  non_literal();
  ~non_literal();
  int value;
};

struct runtime_only_member {
  runtime_only_member();
  runtime_only_member(const runtime_only_member&);
};

struct relaxed_defaulted {
  runtime_only_member member;
  constexpr relaxed_defaulted(const relaxed_defaulted&) = default;
};

constexpr int dead_non_literal(bool use) {
  if (use) {
    non_literal object;
    return object.value;
  }
  return 3;
}

constexpr int dead_static(bool use) {
  if (use) {
    static int value = 4;
    return value;
  }
  return 5;
}

constexpr int label_on_path() {
label:
  return 6;
}

constexpr char hex_digit(int value) {
  static constexpr char digits[] = "0123456789abcdef";
  return digits[value];
}

constexpr non_literal runtime_only_factory() {
  return {};
}

static_assert(dead_non_literal(false) == 3);
static_assert(dead_static(false) == 5);
static_assert(label_on_path() == 6);
static_assert(hex_digit(10) == 'a');

int main() {
  return 0;
}
