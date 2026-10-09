// RUN: -std=c++20
// Pointer objects as lvalues in constant evaluation ([expr.const]): a
// reference to a pointer designates where the pointer is stored, so reads,
// assignments, increments, and `&` through it act on that pointer object,
// including through forwarding references; assignment and compound
// assignment to pointer elements and through pointers to pointers.
#include <memory>
#include <utility>
#include <vector>

template <class T> constexpr T* Id(T*& v) { return v; }
template <class T> constexpr T* IdMoved(T*&& v) { return v; }
template <class T> constexpr void Set(T*& v, T* to) { v = to; }
template <class T> constexpr T** Where(T*& v) { return &v; }
template <class T> constexpr void Bump(T*& v) { ++v; }
template <class T> constexpr void SetMoved(T*&& v, T* to) { Set(v, to); }
template <class T> constexpr void Construct1(T* p, T&& v) {
  std::construct_at(p, static_cast<T&&>(v));
}
template <class T, class... A> constexpr void Construct2(T* p, A&&... a) {
  std::construct_at(p, static_cast<A&&>(a)...);
}
template <class T, class... A> constexpr void Construct3(T* p, A&&... a) {
  Construct2(p, static_cast<A&&>(a)...);
}
template <class T> constexpr void Construct4(T* p, T&& v) {
  Construct2(p, static_cast<T&&>(v));
}

constexpr int ReadThroughReference() {
  int a = 1, b = 2;
  int* x[2] = {&a, &b};
  int* p = &b;
  return *Id(x[0]) * 100 + *IdMoved(std::move(x[1])) * 10 + *Id(p);
}
static_assert(ReadThroughReference() == 122);

constexpr int ForwardedElement() {
  int a = 1, b = 2, c = 3, d = 4;
  int* x[4] = {&a, &b, &c, &d};
  int* y[4] = {};
  Construct1(&y[0], std::move(x[0]));
  Construct2(&y[1], std::move(x[1]));
  Construct3(&y[2], std::move(x[2]));
  Construct4(&y[3], std::move(x[3]));
  return *y[0] * 1000 + *y[1] * 100 + *y[2] * 10 + *y[3];
}
static_assert(ForwardedElement() == 1234);

constexpr int WriteThroughReference() {
  int a = 1, b = 2, c = 3;
  int* x[2] = {&a, &a};
  Set(x[1], &b);
  int* p = &a;
  int*& r = p;
  r = &c;
  return *x[1] * 100 + *x[0] * 10 + *p;
}
static_assert(WriteThroughReference() == 213);

constexpr int WriteThroughForwardedReference() {
  int a = 1, b = 4;
  int* x[1] = {&a};
  SetMoved(std::move(x[0]), &b);
  return *x[0];
}
static_assert(WriteThroughForwardedReference() == 4);

constexpr int WriteThroughLocalReference() {
  int a = 1, b = 6, c = 7;
  int* x[2] = {&a, &a};
  int*& r = x[0];
  r = &b;
  int*& s = r;
  x[1] = &c;
  int*& t = x[1];
  return *s * 10 + *t;
}
static_assert(WriteThroughLocalReference() == 67);

constexpr bool AddressOfReference() {
  int a = 1;
  int* x[2] = {&a, &a};
  int* p = &a;
  int*& r = x[1];
  int*& s = p;
  return Where(x[1]) == &x[1] && &r == &x[1] && Where(r) == &x[1] &&
         &s == &p && Where(s) == &p && &r != &x[0];
}
static_assert(AddressOfReference());

constexpr int IncrementThroughReference() {
  int a[4] = {1, 2, 3, 4};
  int* x[1] = {a};
  int*& r = x[0];
  Bump(x[0]);
  r += 2;
  return *x[0];
}
static_assert(IncrementThroughReference() == 4);

constexpr int PointerElementAssignment() {
  int a = 1, b = 7;
  int* x[1] = {&a};
  x[0] = &b;
  return *x[0] * 10 + a;
}
static_assert(PointerElementAssignment() == 71);

struct Pointers { int* q[2]; };
constexpr int PointerMemberElementAssignment() {
  int a = 1, b = 7;
  Pointers p{{&a, &a}};
  p.q[1] = &b;
  return *p.q[1] * 10 + *p.q[0];
}
static_assert(PointerMemberElementAssignment() == 71);

constexpr int HeapPointerElements() {
  int a = 1, b = 7;
  int** x = new int*[2]{&a, &a};
  x[0] = &b;
  Set(x[1], &b);
  int r = *x[0] * 10 + *x[1];
  delete[] x;
  return r;
}
static_assert(HeapPointerElements() == 77);

constexpr int PointerToPointer() {
  int a[3] = {1, 2, 3}, b = 9;
  int* p = &b;
  int** pp = &p;
  *pp = a;
  ++*pp;
  int r = *p;
  int* x[1] = {&b};
  int** qq = &x[0];
  *qq = &a[2];
  return r * 100 + *x[0] * 10 + (pp == &p && *pp == &a[1]);
}
static_assert(PointerToPointer() == 231);

constexpr int CompoundPointerAssignment() {
  int a[4] = {1, 2, 3, 4};
  int* p = a;
  p += 3;
  p -= 1;
  int* x[1] = {a};
  x[0] += 1;
  return *p * 10 + *x[0];
}
static_assert(CompoundPointerAssignment() == 32);

template <class T> constexpr T Copy(T& v) { return v; }
constexpr int ReferenceArgumentEvaluatedOnce() {
  int b = 5;
  int a[3] = {1, 2, 3};
  int* x[3] = {&b, &b, &b};
  int i = 0, j = 0;
  Copy(a[i++]);
  Id(x[j++]);
  int*& r = x[++j];
  return i * 100 + j * 10 + *r;
}
static_assert(ReferenceArgumentEvaluatedOnce() == 125);

constexpr int ReferenceToPointerTemporary() {
  int a[2] = {1, 2};
  int* p = a;
  int** pp = &p;
  int**&& r1 = &p;
  int* const& r2 = &a[1];
  int* const& r3 = p + 1;
  int**&& r4 = static_cast<int**&&>(pp);
  return **r1 * 1000 + *r2 * 100 + *r3 * 10 + (r4 == pp);
}
static_assert(ReferenceToPointerTemporary() == 1221);

struct Element { int m; int* q; };
constexpr int DifferenceFromArray() {
  Element s[3] = {{1, nullptr}, {2, nullptr}, {3, nullptr}};
  Element* p = s;
  p++;
  int v = (p++)->m;
  int a[2] = {1, 2};
  int* q = a + 1;
  return v * 100 + (int)(p - s) * 10 + (int)(q - a);
}
static_assert(DifferenceFromArray() == 221);

constexpr int AllocatorTraitsMovesPointer() {
  int a = 1;
  std::allocator<int*> al;
  using Traits = std::allocator_traits<std::allocator<int*>>;
  int** p = al.allocate(1);
  Traits::construct(al, p, &a);
  int** q = al.allocate(2);
  Traits::construct(al, q, std::move(*p));
  Traits::destroy(al, p);
  al.deallocate(p, 1);
  int r = *q[0];
  Traits::destroy(al, q);
  al.deallocate(q, 2);
  return r;
}
static_assert(AllocatorTraitsMovesPointer() == 1);

constexpr int VectorOfPointersGrows() {
  int a = 1, b = 2, c = 3;
  std::vector<int*> v;
  v.push_back(&a);
  v.push_back(&b);
  v.push_back(&c);
  v[1] = &c;
  return *v[0] * 100 + *v[1] * 10 + *v[2];
}
static_assert(VectorOfPointersGrows() == 133);

int main() {}
