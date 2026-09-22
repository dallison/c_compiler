// RUN: -std=c++20
// EXPECT: No such symbol "std::list"
// EXPECT: is defined in header <list>
// EXPECT: did you forget to '#include <list>' or 'import std;'

int main() {
  std::list<int> values;
  return 0;
}
