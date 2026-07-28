// RUN: -std=c++23
// EXPECT_EXIT: 0

consteval int immediate(int value) {
  return value + 10;
}

consteval int immediate_forward(int value) {
  return immediate(value);
}

constexpr int select_context(int value) {
  if consteval {
    return immediate(value);
  } else {
    return value + 1;
  }
}

constexpr int select_negated(int value) {
  if ! consteval {
    return value + 2;
  } else {
    return immediate(value);
  }
}

constexpr int transitive_context(int value) {
  return select_context(value) + 1;
}

constexpr int no_else_context() {
  if consteval {
    return 30;
  }
  return 31;
}

constexpr int no_else_negated() {
  if ! consteval {
    return 40;
  }
  return 41;
}

template <class T>
constexpr int template_context(T value) {
  if consteval {
    return immediate(static_cast<int>(value));
  } else {
    return static_cast<int>(value) + 3;
  }
}

static_assert(immediate_forward(1) == 11);
static_assert(select_context(2) == 12);
static_assert(select_negated(3) == 13);
static_assert(template_context(4) == 14);
static_assert(transitive_context(5) == 16);
static_assert(no_else_context() == 30);
static_assert(no_else_negated() == 41);
constexpr int folded_context = select_context(6);
static_assert(folded_context == 16);

int main() {
  if (select_context(20) != 21) {
    return 1;
  }
  if (select_negated(20) != 22) {
    return 2;
  }
  if (template_context(20) != 23) {
    return 3;
  }
  if (transitive_context(20) != 22) {
    return 4;
  }
  if (no_else_context() != 31 || no_else_negated() != 40) {
    return 5;
  }
  return 0;
}
