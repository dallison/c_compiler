// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <string>
#include <string_view>

int main() {
  std::string s = "ab";
  s.append(std::string_view("cd"));
  s.insert(2, std::string_view("XY"));
  s.replace(1, 2, std::string_view("Q"));
  return s == "aQYcd" ? 0 : 1;
}
