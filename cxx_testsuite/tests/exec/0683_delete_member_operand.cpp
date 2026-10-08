// RUN: -std=c++20
// EXPECT_EXIT: 0
// `delete[] s.d` and `delete h.c` on member operands: the operand's type
// decides the array header and the destructor, so the operand must be typed
// before the delete-expression is lowered.  Untyped, `delete[]` passed the
// element pointer rather than the allocation to operator delete[] and `delete`
// skipped the destructor.
#include <cstdlib>
#include <new>

static void* last_new;
static void* last_delete;

void* operator new[](std::size_t n) {
  last_new = std::malloc(n);
  return last_new;
}
void operator delete[](void* p) noexcept {
  last_delete = p;
  std::free(p);
}
void operator delete[](void* p, std::size_t) noexcept {
  last_delete = p;
  std::free(p);
}
void* operator new(std::size_t n) { return std::malloc(n); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }

struct S { int* d; };
struct T { S* s; };

static int destroyed;
struct C {
  int v = 1;
  ~C() { destroyed += v; }
};
struct H { C* c; C* cs; };

int main() {
  H h{new C, new C[3]};
  delete h.c;
  if (destroyed != 1) return 1;
  delete[] h.cs;
  if (destroyed != 4 || last_delete != last_new) return 2;
  H* ph = &h;
  ph->cs = new C[2];
  delete[] ph->cs;
  if (destroyed != 6 || last_delete != last_new) return 3;

  int* local = new int[3];
  delete[] local;
  if (last_delete != last_new) return 4;
  S s;
  s.d = new int[3];
  delete[] s.d;
  if (last_delete != last_new) return 5;
  S* ps = &s;
  ps->d = new int[5];
  delete[] ps->d;
  if (last_delete != last_new) return 6;
  T t{&s};
  t.s->d = new int[2];
  delete[] (t.s->d);
  if (last_delete != last_new) return 7;
  return 0;
}
