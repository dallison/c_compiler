// RUN: -std=c++23

consteval int immediate(int value) {
  return value + 10;
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
    return 20;
  }
  return 21;
}

constexpr int no_else_negated() {
  if ! consteval {
    return 30;
  }
  return 31;
}

template <class T>
constexpr int template_context(T value) {
  if consteval {
    return immediate(static_cast<int>(value));
  } else {
    return static_cast<int>(value) + 3;
  }
}

static_assert(select_context(1) == 11);
static_assert(select_negated(2) == 12);
static_assert(template_context(3) == 13);
static_assert(transitive_context(4) == 15);
static_assert(no_else_context() == 20);
static_assert(no_else_negated() == 31);

int main() {
  return select_context(4) + select_negated(5) + template_context(6);
}
