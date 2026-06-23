// RUN: -std=c++20
struct LongTag {
  long value;
  LongTag(int initial) : value(initial) {
  }
};

template <typename T>
struct ExplicitOnly {
  T value;
};

explicit ExplicitOnly(int) -> ExplicitOnly<LongTag>;

ExplicitOnly copied = 4;
