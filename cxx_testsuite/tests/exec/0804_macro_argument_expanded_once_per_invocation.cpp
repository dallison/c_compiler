// RUN: -std=c++17
// EXPECT_EXIT: 0

// A macro argument is macro-replaced once before substitution, and the result
// replaces every occurrence of the parameter ([cpp.subst]).  `__COUNTER__` in
// an argument therefore yields one value however often the parameter is used
// (protobuf's ASSIGN_OR_RETURN declares and then tests one such name).

#define CAT_I(x, y) x##y
#define CAT(x, y) CAT_I(x, y)
#define DECL(n) int n = 1; total += n;
#define DECL_VA(...) int __VA_ARGS__ = 2; total += __VA_ARGS__;
#define PASTE(a, b) a##b + CAT(a, b)
#define TWICE(x) ((x) + (x))
#define DECL_IMPL(st, lhs, rexpr) int st = (lhs = (rexpr), 0); if (st != 0) return st;

template <typename F>
int Call(F f) { return f(); }

template <typename T>
int InLambda(T v) {
  int out = 0;
  int rc = Call([&out, v]() {
    DECL_IMPL(CAT(_status, __COUNTER__), out, v + 1)
    return 0;
  });
  return rc + out;
}

int main() {
  int total = 0;
  DECL(CAT(_sv, __COUNTER__))
  DECL_VA(CAT(_va, __COUNTER__))
  int ab = 10;
  total += PASTE(a, b);
  int first = __COUNTER__;
  if (TWICE(__COUNTER__) != 2 * (first + 1)) return 1;
  if (total != 1 + 2 + 20) return 2;
  if (InLambda(4) != 5) return 3;
  return 0;
}
