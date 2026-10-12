// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <string>

int main() {
  std::string value = "ac";
  value.insert(1, "b");
  if (value != "abc") return 1;

  value.insert(3, "def", 2);
  if (value != "abcde") return 2;

  value.insert(0, value);
  if (value != "abcdeabcde") return 3;

  value.insert(5, value, 1, 3);
  if (value != "abcdebcdabcde") return 4;
  return 0;
}
