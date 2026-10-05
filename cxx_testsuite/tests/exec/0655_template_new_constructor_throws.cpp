// RUN: -std=c++20
// EXPECT_EXIT: 0

// A new-expression in a template uses the allocated class's own allocation
// functions (unless written `::new`), and when the constructor throws it frees
// the storage with the deallocation function that matches the allocation.
#include <cstdlib>
#include <new>

int allocs = 0, class_allocs = 0, live = 0;
bool should_throw = false;

void* operator new(std::size_t n) { ++allocs; return std::malloc(n); }
void* operator new[](std::size_t n) { ++allocs; return std::malloc(n); }
void operator delete(void* p) noexcept { if (p) { --allocs; std::free(p); } }
void operator delete[](void* p) noexcept { if (p) { --allocs; std::free(p); } }
void operator delete(void* p, std::size_t) noexcept {
  if (p) { --allocs; std::free(p); }
}
void operator delete[](void* p, std::size_t) noexcept {
  if (p) { --allocs; std::free(p); }
}

struct E {};
struct S {
  int v;
  S() : v(1) { if (should_throw) throw E(); ++live; }
  S(int x) : v(x) { if (should_throw) throw E(); ++live; }
  ~S() { --live; }
};

struct C {
  int v;
  C() : v(2) { if (should_throw) throw E(); ++live; }
  C(int x) : v(x) { if (should_throw) throw E(); ++live; }
  ~C() { --live; }
  static void* operator new(std::size_t n) {
    ++class_allocs;
    return std::malloc(n);
  }
  static void operator delete(void* p) {
    if (p) { --class_allocs; std::free(p); }
  }
  static void* operator new[](std::size_t n) {
    ++class_allocs;
    return std::malloc(n);
  }
  static void operator delete[](void* p) {
    if (p) { --class_allocs; std::free(p); }
  }
};

template <class T>
T* make() { return new T; }

template <class T>
T* make_arg(int x) { return new T(x); }

template <class T>
T* make_array(int n) { return new T[n]; }

template <class T>
T* make_array_value(int n) { return new T[n](); }

template <class T>
T* make_global() { return ::new T; }

// Throws while constructing the third element.
struct P {
  P() { if (live == 2) throw E(); ++live; }
  ~P() { --live; }
};

static bool balanced() { return allocs == 0 && class_allocs == 0 && live == 0; }

int main() {
  should_throw = true;
  try { make<S>(); } catch (E&) {}
  if (!balanced()) return 1;
  try { make<C>(); } catch (E&) {}
  if (!balanced()) return 2;
  try { make_arg<C>(3); } catch (E&) {}
  if (!balanced()) return 3;
  try { make_array<C>(3); } catch (E&) {}
  if (!balanced()) return 4;
  try { make_global<C>(); } catch (E&) {}
  if (!balanced()) return 5;
  try { make_array_value<C>(3); } catch (E&) {}
  if (!balanced()) return 12;
  should_throw = false;
  try { make_array<P>(3); } catch (E&) {}
  if (!balanced()) return 13;
  try { make_array_value<P>(3); } catch (E&) {}
  if (!balanced()) return 14;

  should_throw = false;
  S* s = make<S>();
  if (s->v != 1 || live != 1 || allocs != 1) return 6;
  delete s;
  C* c = make<C>();
  if (c->v != 2 || live != 1 || class_allocs != 1 || allocs != 0) return 7;
  delete c;
  C* d = make_arg<C>(4);
  if (d->v != 4 || class_allocs != 1 || allocs != 0) return 8;
  delete d;
  C* a = make_array<C>(2);
  if (live != 2 || class_allocs != 1 || allocs != 0) return 9;
  delete[] a;
  C* g = make_global<C>();
  if (class_allocs != 0 || allocs != 1) return 10;
  ::delete g;
  if (!balanced()) return 11;
  C* av = make_array_value<C>(2);
  if (live != 2 || av[1].v != 2 || class_allocs != 1) return 15;
  delete[] av;
  int* iv = make_array_value<int>(3);
  if (iv[0] != 0 || iv[2] != 0 || allocs != 1) return 16;
  delete[] iv;
  int* ia = make_array<int>(2);
  ia[1] = 5;
  delete[] ia;
  if (!balanced()) return 17;
  return 0;
}
