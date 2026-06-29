// RUN: -std=c++20
// A failed operator/function overload resolution should, like clang and gcc,
// list every candidate considered and explain why each one was rejected.
// EXPECT: No matching overload for operator+
// EXPECT: candidate 'int Vec::operator+(const struct Vec&) const' not viable: no known conversion from 'const char *' to 'const struct Vec&' for argument 1
// EXPECT: candidate 'int Vec::operator+(int ) const' not viable: no known conversion from 'const char *' to 'int ' for argument 1
// EXPECT: No matching overload for pick
// EXPECT: candidate 'void pick(int )' not viable: no known conversion from 'char [3]' to 'int ' for argument 1
// EXPECT: candidate 'void pick(double , double )' not viable: requires 2 arguments, but 1 was provided
struct Vec {
  int operator+(const Vec&) const;
  int operator+(int) const;
};

void pick(int);
void pick(double, double);

void use(void) {
  Vec v;
  const char* text = "";
  v + text;
  pick("hi");
}
