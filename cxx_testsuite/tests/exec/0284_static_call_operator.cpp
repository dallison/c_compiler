// RUN: -std=c++23
// EXPECT_EXIT: 0

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

int constructions;

struct tracked_callable {
  tracked_callable() {
    ++constructions;
  }

  static int operator()(int value) {
    return value + 3;
  }
};

struct nullary_callable {
  nullary_callable() {
    ++constructions;
  }

  static int operator()() {
    return 64;
  }
};

template <class T>
int invoke_temporary(int value) {
  return T{}(value);
}

int main() {
  callable object;
  if (object(10) != 11) {
    return 1;
  }
  if (callable{}(20) != 21) {
    return 2;
  }
  if (callable::operator()(30) != 31) {
    return 3;
  }
  if (invoke_temporary<callable>(40) != 41) {
    return 4;
  }
  indexer indices;
  if (indices[50] != 52 || indexer{}[60] != 62) {
    return 5;
  }
  if (tracked_callable{}(70) != 73 || nullary_callable{}() != 64) {
    return 6;
  }
  if (constructions != 2) {
    return 7;
  }
  return 0;
}
