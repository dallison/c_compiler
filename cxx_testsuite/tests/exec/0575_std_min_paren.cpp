// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <string_view>

int clip(size_t pos, size_t size) {
  pos = (std::min)(pos, size);
  return (int)pos;
}

int main() {
  std::string_view s = "abcd";
  return clip(10, s.size()) == 4 ? 0 : 1;
}
