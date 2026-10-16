// RUN: -std=c++17
// EXPECT_EXIT: 0

// allocator_traits supplies the member types a minimal allocator leaves out
// ([allocator.traits.types]), rebinds by replacing the first template
// argument when there is no rebind member, and is_always_equal follows
// is_empty, which ignores member types, enumerators and static members.
#include <memory>
#include <type_traits>
template <class T> struct Min {
  using value_type = T;
  Min() {}
  template <class U> Min(const Min<U>&) {}
  T* allocate(size_t n) { return static_cast<T*>(::operator new(n * sizeof(T))); }
  void deallocate(T* p, size_t) { ::operator delete(p); }
};
template <class T, class Tag> struct Tagged {
  using value_type = T;
  T* allocate(size_t n) { return static_cast<T*>(::operator new(n * sizeof(T))); }
  void deallocate(T* p, size_t) { ::operator delete(p); }
};
using MT = std::allocator_traits<Min<int>>;
static_assert(std::is_same<MT::pointer, int*>::value, "pointer");
static_assert(std::is_same<MT::const_pointer, const int*>::value, "const_pointer");
static_assert(std::is_same<MT::void_pointer, void*>::value, "void_pointer");
static_assert(std::is_same<MT::const_void_pointer, const void*>::value, "cvp");
static_assert(std::is_same<MT::difference_type, ptrdiff_t>::value, "diff");
static_assert(std::is_same<MT::size_type, size_t>::value, "size");
static_assert(std::is_same<MT::rebind_alloc<long>, Min<long>>::value, "rebind");
static_assert(std::is_same<std::allocator_traits<Tagged<int, char>>::rebind_alloc<long>, Tagged<long, char>>::value, "rebind2");
static_assert(MT::is_always_equal::value, "empty");
static_assert(std::is_same<std::allocator_traits<std::allocator<int>>::rebind_alloc<long>, std::allocator<long>>::value, "std");
static_assert(std::is_same<MT::rebind_traits<char>::pointer, char*>::value, "rebind_traits");
struct E {};
struct F { F() {} int f() { return 0; } };
template <class T> struct G { using value_type = T; G() {} template <class U> G(const G<U>&) {} };
static_assert(std::is_empty<E>::value, "E");
static_assert(std::is_empty<F>::value, "F");
static_assert(std::is_empty<G<int>>::value, "G");
template <class A, class = void> struct W { using type = typename std::is_empty<A>::type; };
static_assert(W<G<int>>::type::value, "W");
static_assert(std::is_same<std::is_empty<E>::type, std::true_type>::value, "type");
struct H { enum { A, B }; using X = int; struct N { int n; }; static int s; static constexpr int c = 1; };
static_assert(std::is_empty<H>::value, "H");
struct I { int : 0; };
struct J { union { int u; }; };
static_assert(!std::is_empty<J>::value, "J");
struct K : G<int> {};
static_assert(std::is_empty<K>::value, "K");
struct L { virtual void f(); };
static_assert(!std::is_empty<L>::value, "L");
static_assert(std::is_empty<I>::value, "I");
struct M { int b : 3; };
static_assert(!std::is_empty<M>::value, "M");
struct N2 { struct { int x; }; };
static_assert(!std::is_empty<N2>::value, "N2");
int main() {
  Min<int> a;
  int* p = MT::allocate(a, 4, nullptr);
  MT::construct(a, p, 5);
  int v = *p;
  MT::destroy(a, p);
  MT::deallocate(a, p, 4);
  return v == 5 && MT::max_size(a) == static_cast<size_t>(-1) / sizeof(int) ? 0 : 1;
}
