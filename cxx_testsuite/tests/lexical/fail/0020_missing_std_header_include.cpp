// RUN: -std=c++17
// EXPECT: No such symbol "std::list"
// EXPECT: is defined in header <list>
// EXPECT: did you forget to '#include <list>'

int main() {
  std::list<int> values;
  return 0;
}
