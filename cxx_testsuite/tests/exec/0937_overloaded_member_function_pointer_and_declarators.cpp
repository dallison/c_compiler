// `&C::f` naming an overload set selects the member whose type matches the
// target pointer-to-member type ([over.over]), in initialization, assignment,
// aggregate members and function arguments.  Also cv-qualified member pointers
// (`int (C::* const p)(int)`) and arrays of member pointers.
#include <stdio.h>

struct Foo {
  int n = 0;
  int a = 1, b = 2;
  void Bump() { n += 1; }
  int Bump(int k) { return n += k; }
  int Bump(int k, int m) const { return k * m; }
  int Get() const { return a; }
  int Get2() const { return b; }
};

struct Holder {
  int (Foo::*p)(int);
};

template <class M>
struct Const {
  const M p;
};

static int CallWith(Foo& f, int (Foo::*p)(int), int k) { return (f.*p)(k); }

int main() {
  Foo f;
  int (Foo::*p)(int) = &Foo::Bump;
  (f.*p)(10);
  void (Foo::*q)() = nullptr;
  q = &Foo::Bump;
  (f.*q)();
  Holder h{&Foo::Bump};
  (f.*h.p)(100);
  Const<int (Foo::*)(int, int) const> c{&Foo::Bump};
  int product = (f.*c.p)(6, 7);
  int via_arg = CallWith(f, &Foo::Bump, 1000);

  int Foo::*members[2] = {&Foo::a, &Foo::b};
  int (Foo::* const cp)() const = &Foo::Get2;
  int Foo::* const dp = &Foo::b;
  int (Foo::*getters[2])() const = {&Foo::Get, &Foo::Get2};
  int mix = f.*members[0] + f.*members[1] * 10 + (f.*cp)() * 100 +
            f.*dp * 1000 + (f.*getters[1])() * 10000;
  int count = (int)(sizeof(members) / sizeof(members[0]));

  if (f.n != 1111 || product != 42 || via_arg != 1111 || mix != 22221 ||
      count != 2) {
    printf("%d %d %d %d %d\n", f.n, product, via_arg, mix, count);
    return 1;
  }
  printf("ok\n");
  return 0;
}
