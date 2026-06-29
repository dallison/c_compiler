// RUN: -std=c++20
//
// A class may grant friendship before the friend is declared (forward friend
// class), to free functions, and to inline-defined operators.

class Number {
  int value;

 public:
  Number(int v) : value(v) {}

  friend class Printer;          // Forward-declared friend class.
  friend bool equal(const Number& a, const Number& b);  // Friend function.
  friend Number operator*(const Number& a, const Number& b) {  // Inline friend.
    return Number(a.value * b.value);
  }
};

class Printer {
 public:
  int get(const Number& n) const { return n.value; }
};

bool equal(const Number& a, const Number& b) { return a.value == b.value; }

int use(void) {
  Number a(6);
  Number b(7);
  Printer p;
  Number c = a * b;
  return (equal(a, b) ? 0 : p.get(c));
}
