// Local classes of one function that share a name are distinct types: each
// has its own members, and templates instantiated with them are distinct
// specializations (Itanium discriminators `Local`, `Local_0`, ...).
#include <stdio.h>

struct Obj {};

template <class F> int Call(F f) {
  Obj o;
  return f(static_cast<const Obj&>(o));
}

template <class F> struct Box {
  F f;
  int Run() const {
    Obj o;
    return f(o);
  }
};

struct Base {
  virtual ~Base() {}
  virtual int Get() const = 0;
};

namespace ns {
int Make(int which) {
  Base* b;
  if (which == 0) {
    struct Impl : Base {
      int Get() const override { return 10; }
    };
    b = new Impl;
  } else {
    struct Impl : Base {
      int Get() const override { return 20; }
    };
    b = new Impl;
  }
  int r = b->Get();
  delete b;
  return r;
}
}  // namespace ns

#define BLOCK(n)                               \
  {                                            \
    struct K {                                 \
      int f() const { return n; }              \
    };                                         \
    sum = sum * 3 + K().f();                   \
  }

int Many() {
  int sum = 0;
  BLOCK(1) BLOCK(2) BLOCK(3) BLOCK(4) BLOCK(5) BLOCK(6)
  BLOCK(7) BLOCK(8) BLOCK(9) BLOCK(10) BLOCK(11) BLOCK(12)
  return sum;
}

int main() {
  int failures = 0;
  int a, b, c, d;
  {
    struct Local {
      int operator()(Obj&) const { return 1; }
      int operator()(const Obj&) const { return 2; }
    };
    a = Call(Local{});
    c = Box<Local>{Local{}}.Run();
  }
  {
    struct Local {
      int operator()(Obj&) const { return 3; }
      int operator()(const Obj&) const { return 4; }
    };
    b = Call(Local{});
    d = Box<Local>{Local{}}.Run();
  }
  if (a != 2 || b != 4 || c != 1 || d != 3) failures |= 1;
  if (ns::Make(0) != 10 || ns::Make(1) != 20) failures |= 2;
  if (Many() != 398574) failures |= 4;
  if (failures == 0) printf("ok\n");
  return failures;
}
