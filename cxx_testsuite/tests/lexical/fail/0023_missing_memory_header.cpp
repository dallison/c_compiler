// RUN: -std=c++20
// EXPECT: No such symbol "std::unique_ptr"
// EXPECT: is defined in header <memory>
// EXPECT: did you forget to '#include <memory>' or 'import std;'

int main() {
  std::unique_ptr<int> pointer;
  return 0;
}
