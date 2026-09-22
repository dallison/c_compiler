// RUN: -std=c++17
// EXPECT: No such symbol "std::string"
// EXPECT: is defined in header <string>
// EXPECT: did you forget to '#include <string>'

int main() {
  std::string text;
  return 0;
}
