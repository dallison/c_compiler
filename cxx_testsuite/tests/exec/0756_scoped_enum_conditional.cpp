// RUN: -std=c++17
// EXPECT_EXIT: 0

enum class Mode : unsigned char {
  first = 3,
  second = 7,
};

constexpr bool AlwaysFirst() {
  return true;
}

Mode SelectMode(bool select_first) {
  if constexpr (AlwaysFirst()) {
    return Mode::first;
  }
  return select_first ? Mode::first : Mode::second;
}

int main() {
  if (SelectMode(true) != Mode::first) {
    return 1;
  }
  return SelectMode(false) == Mode::first ? 0 : 2;
}
