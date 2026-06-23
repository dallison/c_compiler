// RUN: -std=c++20
// EXPECT: Explicit deduction guide cannot be used for copy-initialization

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

void fail_explicit_copy_list_ctad() {
  ExplicitOnly copied = {4};
}
