// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <string>

int main() {
  const std::string path = "one/two.name";
  if (path.find_last_of("./") != 7) {
    return 1;
  }
  if (path.find_last_of("./", 6) != 3) {
    return 2;
  }
  if (path.find_last_of(std::string("en")) != 11) {
    return 3;
  }
  if (path.find_last_of('o') != 6) {
    return 4;
  }
  if (path.find_last_of("", std::string::npos) != std::string::npos) {
    return 5;
  }
  return std::string().find_last_of("x") == std::string::npos ? 0 : 6;
}
