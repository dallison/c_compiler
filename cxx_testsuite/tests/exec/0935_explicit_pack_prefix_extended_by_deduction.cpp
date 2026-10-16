// Template argument deduction extends a parameter pack past its explicitly
// specified elements ([temp.arg.explicit]/9).  The explicit elements keep
// their types; only the remaining call arguments are deduced.
#include <stdio.h>

template <typename R, typename... A>
R Count(A...) {
  return (R)sizeof...(A);
}

template <typename... A>
int Sizes(A... a) {
  int sizes[] = {(int)sizeof(a)..., 0};
  int total = 0;
  for (int i = 0; i < (int)sizeof...(A); i++) total = total * 10 + sizes[i];
  return total;
}

template <typename R, typename... A, typename... B>
R Split(void (*)(A...), B...) {
  return (R)(sizeof...(A) * 10 + sizeof...(B));
}

void Two(int, int) {}

struct S {
  template <typename R, typename... A>
  R Count(A...) const {
    return (R)sizeof...(A);
  }
};

template <class... T>
int Dependent() {
  return Count<int, T...>(T()..., 1.5);
}

int main() {
  int a = Count<int, int>(1, 'c', 1.5);
  int b = S().Count<int, int, char>(1, 'c', 1.5, 2);
  int c = Count<int, int, char>(1, 'c');
  int d = Sizes<long long>(1, 'c');
  int e = Sizes<char, char>(1, 2, 3LL);
  int f = Split<int, int>(Two, 1, 2, 3);
  int g = Dependent<int, char>();
  if (a != 3 || b != 4 || c != 2 || d != 81 || e != 118 || f != 23 ||
      g != 3) {
    printf("%d %d %d %d %d %d %d\n", a, b, c, d, e, f, g);
    return 1;
  }
  printf("ok\n");
  return 0;
}
