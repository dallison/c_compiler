// A local pointer to member function initialized with nullptr holds a real
// null value, and one initialized with a member keeps any later assignment:
// calls through it must not be folded to the initializer.
#include <stdio.h>

struct Foo {
  int n = 0;
  void Bump() { n += 1; }
  void Other() { n += 5; }
};

static int CallTwice(Foo& f, bool swap) {
  void (Foo::*q)() = &Foo::Other;
  if (swap) q = &Foo::Bump;
  (f.*q)();
  (f.*q)();
  return f.n;
}

int main() {
  Foo f;
  void (Foo::*null_init)() = nullptr;
  bool was_null = null_init == nullptr && !null_init;
  null_init = &Foo::Bump;
  bool now_set = null_init != nullptr;
  (f.*null_init)();

  void (Foo::*reassigned)() = &Foo::Other;
  reassigned = &Foo::Bump;
  (f.*reassigned)();

  void (Foo::* const fixed)() = &Foo::Other;
  (f.*fixed)();

  Foo g;
  Foo h;
  int swapped = CallTwice(g, true);
  int kept = CallTwice(h, false);

  if (!was_null || !now_set || f.n != 7 || swapped != 2 || kept != 10) {
    printf("%d %d %d %d %d\n", was_null, now_set, f.n, swapped, kept);
    return 1;
  }
  printf("ok\n");
  return 0;
}
