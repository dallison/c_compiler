// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <memory>

int main() {
  auto chars = std::make_unique<char[]>(4);
  chars[0] = 'a';
  chars[1] = 'b';
  auto value = std::make_unique<int>(7);
  return chars[0] == 'a' && chars[1] == 'b' && *value == 7 ? 0 : 1;
}
