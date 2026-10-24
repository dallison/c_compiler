// RUN: -std=c++17
// EXPECT_EXIT: 0

// vector's fill constructor defaults its allocator argument to Allocator().
// That expression still names the class template parameter until the
// constructor body is instantiated. A lambda converted to std::function is
// instantiated while vector itself is still being instantiated, so the call
// sees the unsubstituted default. The constructed vector must still be the
// fill constructor's result.

#include <functional>
#include <stddef.h>
#include <vector>

template <class T>
struct Box {
  template <class U>
  static int id(U value = U()) {
    return (int)sizeof(value) + (int)value;
  }
};

int main() {
  size_t n = 4;
  int result = 1;
  std::function<void()> cb = [&] {
    std::vector<char> seen(n, 7);
    result = seen.size() == 4 && seen[0] == 7 && seen[3] == 7 ? 0 : 2;
  };
  cb();
  if (result != 0) {
    return result;
  }
  if (Box<int>::id<char>() != 1) {
    return 3;
  }
  return 0;
}
