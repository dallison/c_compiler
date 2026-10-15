// Members of anonymous unions and structs are written and read through the
// anonymous member that holds them, and a constant object's image keeps them
// (protobuf's MessageCreator holds `union { Func func_; uintptr_t arena_bits_; }`).
// That includes an anonymous struct that is the active member of an anonymous
// union, and an assignment in the constructor body that changes the active
// member.  The values are read through pointers so they come from the emitted
// data rather than from constant folding.
#include <stdint.h>

struct M {
  constexpr M(int v) : a(v), b(v * 2) {}
  constexpr M(void* p, int v) : a(v), f(p) {}
  int a;
  union {
    void* f;
    unsigned long b;
  };
};
struct S {
  constexpr S(int x) : tag(1), x(x), y(x + 1) {}
  int tag;
  struct {
    int x;
    int y;
  };
  int tail = 99;
};
struct N {
  constexpr N(int v) : k(v), p(v + 1), q(v + 2) {}
  int k;
  union {
    struct {
      short p;
      short q;
    };
    int64_t whole;
  };
};
struct Sw {
  constexpr Sw() : i(1) { d = 2.5; }
  union {
    int i;
    double d;
  };
};

int target;
constexpr M m{5};
constexpr M mp{&target, 3};
constexpr S s{7};
constexpr N n{10};
constexpr Sw sw{};
static_assert(m.a == 5 && m.b == 10, "");
static_assert(s.tag == 1 && s.x == 7 && s.y == 8 && s.tail == 99, "");
static_assert(n.k == 10 && n.p == 11 && n.q == 12, "");
static_assert(sw.d == 2.5, "");
extern const M gm;
const M gm{4};

template <typename T>
const T* Launder(const T* p) {
  const T* volatile q = p;
  return q;
}

int main() {
  if (Launder(&m)->a != 5 || Launder(&m)->b != 10) return 1;
  if (Launder(&mp)->f != &target || Launder(&mp)->a != 3) return 2;
  const S* ps = Launder(&s);
  if (ps->tag != 1 || ps->x != 7 || ps->y != 8 || ps->tail != 99) return 3;
  const N* pn = Launder(&n);
  if (pn->k != 10 || pn->p != 11 || pn->q != 12) return 4;
  if (Launder(&sw)->d != 2.5) return 5;
  if (Launder(&gm)->a != 4 || Launder(&gm)->b != 8) return 6;
  return 0;
}
