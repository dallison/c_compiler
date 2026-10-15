// Macro arguments are replaced with the macro still enabled, a function-like
// macro name ending a replacement takes its arguments from the tokens after
// it (6.10.3.4), and # keeps the white space after commas in __VA_ARGS__ but
// drops it at either end (6.10.3.2).  gmock-pp.h depends on all three.  Only
// white space between the parentheses passes no arguments (gtest-port.h).
#include <string.h>

#define STR2(...) #__VA_ARGS__
#define STR(...) STR2(__VA_ARGS__)

#define CAT(a, b) CAT_I(a, b)
#define CAT_I(a, b) a##b
#define IF_0(t, e) e
#define IF_1(t, e) t
#define IF(c, t, e) CAT(IF_, c)(t, e)
#define SEL(c) CAT(IF_, c)
#define R_IF_1 done
#define COUNT_IMPL_0(...) zero
#define COUNT_IMPL_2(...) two
#define NARG(...) NARG_I(__VA_ARGS__, 2, 1, 0)
#define NARG_I(a, b, n, ...) n
#define FOR_EACH(t) CAT(COUNT_IMPL_, IF(1, NARG t, 0))(t)
#define REMOVE_PARENS(...) REMOVE_PARENS_I __VA_ARGS__
#define REMOVE_PARENS_I(...) __VA_ARGS__
#define PAIR(a, b) a b

#define f(a) a*g
#define g(a) f(a)
#define h(x) x h
#define OBJ FN
#define FN(x) [x]
#define NOP(x) x
#define ID(x) x
#define A(x) ID(x) NOP
#define NONE() none

static int check(const char* got, const char* want) {
  return strcmp(got, want) == 0 ? 0 : 1;
}

int main() {
  int failures = 0;
  failures += check(STR(CAT(R_, SEL(1))), "done");
  failures += check(STR(IF(0, yes, no)), "no");
  failures += check(STR(FOR_EACH((a, b))), "two");
  failures += check(STR(REMOVE_PARENS((int, x))), "int, x");
  failures += check(STR(PAIR((int, x), )), "(int, x)");
  failures += check(STR( a ,  b , c ), "a , b , c");
  failures += check(STR(f(2)(9)), "2*9*g");
  failures += check(STR(h(1)(2)), "1 h(2)");
  failures += check(STR(OBJ(3)), "[3]");
  failures += check(STR(NOP(OBJ)(4)), "[4]");
  failures += check(STR(FN x FN(5)), "FN x [5]");
  failures += check(STR(A(1)(2)), "1 2");
  failures += check(STR(ID(ID)(7)), "ID(7)");
  failures += check(STR(NOP x ID(y)), "NOP x y");
  failures += check(STR(NONE( ) NONE(/* no arguments */) NONE(
                    )), "none none none");
  return failures;
}
