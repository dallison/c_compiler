// RUN: -std=c++23
// EXPECT_EXIT: 0

#include <generator>
#include <utility>

std::generator<int> numbers() {
  co_yield 1;
  int middle = 2;
  co_yield middle;
  co_yield 3;
}

std::generator<int&> references(int* values, int count) {
  for (int i = 0; i < count; ++i) {
    co_yield values[i];
  }
}

struct lifetime {
  int* destroyed;

  ~lifetime() {
    ++*destroyed;
  }
};

std::generator<int> tracked(int* destroyed) {
  lifetime guard{destroyed};
  co_yield 10;
  co_yield 20;
}

#ifdef __cpp_exceptions
struct generator_error {};

std::generator<int> failing() {
  co_yield 1;
  throw generator_error{};
}
#endif

int main() {
  int sum = 0;
  for (int value : numbers()) {
    sum += value;
  }
  if (sum != 6) {
    return 1;
  }

  auto source = numbers();
  auto moved = std::move(source);
  sum = 0;
  for (int value : moved) {
    sum += value;
  }
  if (sum != 6) {
    return 2;
  }

  int values[] = {4, 5, 6};
  for (int& value : references(values, 3)) {
    value *= 2;
  }
  if (values[0] != 8 || values[1] != 10 || values[2] != 12) {
    return 3;
  }

  int destroyed = 0;
  {
    auto sequence = tracked(&destroyed);
    auto iterator = sequence.begin();
    if (*iterator != 10 || destroyed != 0) {
      return 4;
    }
  }
  if (destroyed != 1) {
    return 5;
  }

  auto assigned = numbers();
  auto replacement = numbers();
  assigned = std::move(replacement);
  sum = 0;
  for (int value : assigned) {
    sum += value;
  }
  if (sum != 6) {
    return 6;
  }

#ifdef __cpp_exceptions
  int yielded = 0;
  bool caught = false;
  try {
    for (int value : failing()) {
      yielded += value;
    }
  } catch (const generator_error&) {
    caught = true;
  }
  if (yielded != 1 || !caught) {
    return 7;
  }
#endif

  return 0;
}
