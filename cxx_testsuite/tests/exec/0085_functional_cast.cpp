// Functional-cast syntax `T(arg)` and its disambiguation from a cast.
//
// A single-argument functional cast is equivalent to the explicit type
// conversion `(T)arg` ([expr.type.conv]).  The redundant-parenthesized form
// `(T(arg))` exercises the parser's cast vs. functional-cast disambiguation:
// `T(arg)` is not a type-id, so `(T(arg))` is a parenthesized expression, not a
// cast to a function type.  Casts to real types (including function pointers)
// must keep parsing as casts.

struct Box {
  int v;
  Box(int x) : v(x) {}
};

typedef int (*IntFn)(int);
static int Inc(int x) { return x + 1; }

int main() {
  // Functional cast invoking a constructor.
  Box a = Box(5);
  if (a.v != 5) return 1;

  // Redundant-parenthesized functional cast (previously a parse error: the
  // disambiguator committed to a cast and mis-parsed `Box(7)` as a type).
  Box b = (Box(7));
  if (b.v != 7) return 2;

  // Nested into a larger expression.
  int sum = Box(10).v + (Box(20)).v;
  if (sum != 30) return 3;

  // A real cast to a function-pointer type must still parse as a cast
  // (non-regression for the disambiguation).
  IntFn g = (IntFn)&Inc;
  if (g(41) != 42) return 4;

  // A redundant-parenthesized primitive cast is still a cast.
  double d = 3.9;
  int n = ((int)d);
  if (n != 3) return 5;

  return 0;
}
