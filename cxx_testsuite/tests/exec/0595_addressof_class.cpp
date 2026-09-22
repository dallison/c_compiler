// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <memory>

struct FormatArgImpl {
  int tag;
  void* data;
};

int main() {
  FormatArgImpl arg{1, 0};
  return std::addressof(arg) == &arg ? 0 : 1;
}
