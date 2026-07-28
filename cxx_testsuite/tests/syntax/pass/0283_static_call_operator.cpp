// RUN: -std=c++23

struct callable {
  static constexpr int operator()(int value) {
    return value + 1;
  }
};

struct indexer {
  static constexpr int operator[](int value) {
    return value + 2;
  }
};

template <class T>
constexpr int invoke_temporary(int value) {
  return T{}(value);
}

static_assert(callable{}(4) == 5);
static_assert(callable::operator()(5) == 6);
static_assert(invoke_temporary<callable>(6) == 7);
static_assert(indexer{}[7] == 9);

int main() {
  callable object;
  return object(7);
}
