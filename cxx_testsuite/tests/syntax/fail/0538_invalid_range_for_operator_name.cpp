// RUN: -std=c++11
// EXPECT: cannot be the name of a variable or data member
// An unknown range-declaration type followed by an operator-name must recover
// as a range-for instead of repeatedly parsing the colon as a new statement.
void test() {
  for (a operator== :)
}
