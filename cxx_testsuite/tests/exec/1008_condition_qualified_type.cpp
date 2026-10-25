// RUN: -std=c++17
// EXPECT_EXIT: 0

// A condition may declare a variable whose type is a qualified name
// (`if (::n::Buf* pb = ...)`).

namespace n {
struct Buf {
  int v;
};
}  // namespace n

int main() {
  n::Buf storage;
  storage.v = 3;
  if (::n::Buf* pb = &storage) {
    return pb->v == 3 ? 0 : 1;
  }
  return 2;
}
