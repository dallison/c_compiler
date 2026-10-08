// RUN: -std=c++20
// EXPECT_EXIT: 0
// new-expressions whose type-id is a pointer or an array type ([expr.new]):
// `new int*(&g)` initializes the pointer (its declarator has no parentheses,
// so `(&g)` is the initializer), and `new int*[2]` or `new A` for an array
// typedef A is an array new, with the element count ahead of the elements as
// delete[] expects.  A const object is initialized, not assigned.
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

typedef int A3[3];
static int destroyed;
struct C {
  int v;
  C() : v(1) {}
  ~C() { destroyed += v; }
};
typedef C C2[2];

int g = 7;

int main() {
  int** p = new int*(&g);
  if (*p != &g || **p != 7) return 1;
  delete p;
  const int* const* cp = new const int* const(&g);
  if (**cp != 7) return 2;
  delete cp;
  const int* ci = new const int(5);
  const int* ca = new const int[3]{1, 2, 3};
  if (*ci + ca[2] != 8) return 12;
  delete ci;
  delete[] ca;

  int** ps = new int*[2];
  if (last_new == ps) return 3;
  delete[] ps;
  if (last_delete != last_new) return 4;
  int a = 1, b = 2;
  int** pb = new int*[2]{&a, &b};
  if (*pb[0] + *pb[1] != 3) return 5;
  delete[] pb;
  if (last_delete != last_new) return 6;

  int* q = new A3;
  q[2] = 5;
  delete[] q;
  if (last_delete != last_new) return 7;
  int* qi = new A3{1, 2, 3};
  if (qi[0] + qi[1] + qi[2] != 6) return 8;
  delete[] qi;
  if (last_delete != last_new) return 9;

  C* c = new C2;
  if (c[1].v != 1) return 10;
  delete[] c;
  if (last_delete != last_new || destroyed != 2) return 11;
  return 0;
}
