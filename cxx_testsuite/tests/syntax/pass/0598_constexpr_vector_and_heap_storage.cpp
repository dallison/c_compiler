// RUN: -std=c++20
// std::vector and allocated storage in constant evaluation ([expr.const],
// [vector.overview]): growth moves class elements into new storage, element
// references and pointers into allocations behave as for any array, and
// array new-expressions keep their element count apart from the elements.
#include <memory>
#include <utility>
#include <vector>

struct P {
  int x;
  int y;
  constexpr P(int a, int b) : x(a), y(b) {}
};
struct Q { int a = 7; };

constexpr int Grow() {
  std::vector<P> v;
  v.emplace_back(1, 2);
  v.emplace_back(3, 4);
  v.push_back(P(5, 6));
  int s = 0;
  for (auto& p : v) s += p.x * p.y;
  return s;
}
static_assert(Grow() == 44);

constexpr int Resize() {
  std::vector<Q> v(3);
  v.resize(5);
  return v[4].a + (int)v.size();
}
static_assert(Resize() == 12);

constexpr int ReserveThenGrow() {
  std::vector<int> v;
  v.reserve(1);
  v.emplace_back(-3);
  v.reserve(2);
  v.push_back(300);
  return v[0] + v[1] + (int)v.capacity();
}
static_assert(ReserveThenGrow() == 299);

constexpr int VectorOfPointers() {
  int a = 4, b = 5;
  std::vector<int*> v;
  v.push_back(&a);
  v.push_back(&b);
  *v[1] += 10;
  return *v[0] + b;
}
static_assert(VectorOfPointers() == 19);

// Construction and moves through allocator_traits, as std::vector does.
using AT = std::allocator_traits<std::allocator<P>>;
constexpr int MoveIntoNewStorage() {
  std::allocator<P> al;
  P* p = AT::allocate(al, 1);
  AT::construct(al, p, 1, 2);
  P* q = AT::allocate(al, 2);
  AT::construct(al, q, std::move(*p));
  AT::destroy(al, p);
  AT::deallocate(al, p, 1);
  AT::construct(al, q + 1, 3, 4);
  int r = q[1].y + q[0].x;
  AT::destroy(al, q);
  AT::destroy(al, q + 1);
  AT::deallocate(al, q, 2);
  return r;
}
static_assert(MoveIntoNewStorage() == 5);

// A reference returned into allocated storage reads the element, whether the
// storage is a single object or an array, at any element.
constexpr P& Id(P& v) { return v; }
constexpr int CopyThroughReference() {
  P* one = new P(1, 2);
  P* many = new P[2]{{3, 4}, {5, 6}};
  P a(Id(*one));
  P b(Id(many[0]));
  P c(Id(many[1]));
  delete one;
  delete[] many;
  return a.y + b.y + c.y;
}
static_assert(CopyThroughReference() == 12);

struct R {
  int* d;
  constexpr const int& At(int i) const { return d[i]; }
};
constexpr long ReferenceToElement() {
  int* p = new int[3]{1, -2, 3};
  R r{p};
  long v = r.At(1) + r.At(2);
  delete[] p;
  return v;
}
static_assert(ReferenceToElement() == 1);

// Pointer arithmetic counts elements and may reach one past the end.
constexpr int PointerArithmetic() {
  int* p = new int[4]{1, 2, 3, 4};
  int* end = p + 4;
  int s = 0;
  for (int* q = p; q != end; ++q) s += *q;
  int last = *(end - 1);
  delete[] p;
  return s * 10 + last;
}
static_assert(PointerArithmetic() == 104);

// An array new-expression in a mem-initializer, deleted by the destructor.
struct D {
  int* d;
  constexpr D() : d(new int[3]{1, 2, 3}) {}
  constexpr ~D() { delete[] d; }
};
constexpr int OwnedArray() {
  D x;
  return x.d[2];
}
static_assert(OwnedArray() == 3);

struct S { int* d; };
constexpr int MemberArrayDelete() {
  S s;
  s.d = ({ int* q = new int[3]{1, 2, 3}; q; });
  int r = s.d[2];
  delete[] s.d;
  return r;
}
static_assert(MemberArrayDelete() == 3);

// std::move of a stored pointer yields the pointer, not where it is stored.
constexpr bool MoveStoredPointer() {
  int a = 1;
  int* p[2] = {&a, nullptr};
  int* y = std::move(p[0]);
  return y == &a;
}
static_assert(MoveStoredPointer());

constexpr bool ConstructFromMovedPointer() {
  int a = 1;
  int** p = new int*[1]{&a};
  int** q = new int*[1];
  std::construct_at(q, std::move(p[0]));
  bool r = q[0] == &a && std::move(p[0]) == &a;
  delete[] p;
  delete[] q;
  return r;
}
static_assert(ConstructFromMovedPointer());

int main() { return 0; }
