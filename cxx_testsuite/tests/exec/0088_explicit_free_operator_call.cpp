// Explicit (named) calls of free (non-member) operator functions:
//   operator+(a, b), ::operator+(a, b), ns::operator-(a, b)
// An unqualified operator-function-id is a valid primary expression naming the
// free operator function; qualified forms must keep working too.  Overload
// resolution applies at the call site like any other free function.

struct V {
  int x;
};

V operator+(V a, V b) { return V{a.x + b.x}; }
V operator-(V a, V b) { return V{a.x - b.x}; }
bool operator==(V a, V b) { return a.x == b.x; }
V operator*(V a, int s) { return V{a.x * s}; }
V operator*(int s, V a) { return V{a.x * s}; }

namespace ns {
V operator-(V a, V b) { return V{a.x - b.x + 1}; }
}

int main() {
  V a{10}, b{3};

  // Unqualified explicit calls.
  if (operator+(a, b).x != 13) return 1;
  if (operator-(a, b).x != 7) return 2;
  if (!operator==(a, a)) return 3;
  if (operator==(a, b)) return 4;

  // Overload resolution between the two operator* free functions.
  if (operator*(a, 2).x != 20) return 5;
  if (operator*(5, b).x != 15) return 6;

  // Qualified forms must still resolve.
  if (::operator+(a, b).x != 13) return 7;
  if (ns::operator-(a, b).x != 8) return 8;

  // The implicit operator syntax must still work.
  if ((a + b).x != 13) return 9;
  if (!(a == a)) return 10;
  if ((a * 2).x != 20) return 11;

  return 0;
}
