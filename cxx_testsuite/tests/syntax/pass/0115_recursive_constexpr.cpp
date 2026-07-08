// RUN: -std=c++11
// Recursive constexpr functions must be constant-evaluable.  Regression: a
// recursive call reached while the function's own body was still being analyzed
// (e.g. a self-call inside a ?: operator whose common type was not yet computed)
// triggered speculative pcode lowering of the incomplete body and crashed code
// generation on an untyped node.  The evaluator now recovers and defers folding
// until the definition is complete.

constexpr int fact(int n) { return n <= 1 ? 1 : n * fact(n - 1); }
constexpr int fib(int n) { return n < 2 ? n : fib(n - 1) + fib(n - 2); }
constexpr int gcd(int a, int b) { return b == 0 ? a : gcd(b, a % b); }
constexpr int pow2(int n) { return n == 0 ? 1 : 2 * pow2(n - 1); }

// Recursive call in a ternary branch (both arms same type) -- the crashing case.
constexpr int self(int n) { return n <= 0 ? 0 : self(n - 1); }

// Mutually recursive constexpr functions through a forward declaration.
constexpr int ping(int n);
constexpr int pong(int n) { return n == 0 ? 0 : ping(n - 1) + 1; }
constexpr int ping(int n) { return n == 0 ? 0 : pong(n - 1) + 1; }

static_assert(fact(5) == 120, "fact");
static_assert(fib(10) == 55, "fib");
static_assert(gcd(48, 18) == 6, "gcd");
static_assert(pow2(10) == 1024, "pow2");
static_assert(self(7) == 0, "self");
static_assert(ping(6) == 6, "mutual");

// Recursion used in a constant context (array bound).
char buffer[fact(4)];
static_assert(sizeof(buffer) == 24, "array bound");

int main() { return 0; }
