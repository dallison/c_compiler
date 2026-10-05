// RUN: -std=c++20
// EXPECT_EXIT: 0

// When the initialization in a new-expression throws, the storage is freed
// and the array elements already constructed are destroyed.
#include <cstdlib>
#include <new>

int allocs = 0, live = 0, throw_at = -1;

void* operator new(std::size_t n) { ++allocs; return std::malloc(n); }
void* operator new[](std::size_t n) { ++allocs; return std::malloc(n); }
void operator delete(void* p) noexcept { if (p) { --allocs; std::free(p); } }
void operator delete[](void* p) noexcept { if (p) { --allocs; std::free(p); } }
void operator delete(void* p, std::size_t) noexcept { if (p) { --allocs; std::free(p); } }
void operator delete[](void* p, std::size_t) noexcept { if (p) { --allocs; std::free(p); } }

struct E {};
struct S {
  int v;
  S() : v(0) { if (throw_at == live) throw E(); ++live; }
  S(int x) : v(x) { if (throw_at == live) throw E(); ++live; }
  ~S() { --live; }
};

int main() {
  throw_at = 0;
  try { new S(1); } catch (E&) {}
  if (allocs != 0 || live != 0) return 1;
  try { new S; } catch (E&) {}
  if (allocs != 0 || live != 0) return 2;
  throw_at = 2;
  try { new S[4]; } catch (E&) {}
  if (allocs != 0 || live != 0) return 3;
  try { new S[4]{1, 2, 3, 4}; } catch (E&) {}
  if (allocs != 0 || live != 0) return 4;
  int n = 5;
  try { new S[n]; } catch (E&) {}
  if (allocs != 0 || live != 0) return 5;
  throw_at = -1;
  S* p = new S[3];
  S* q = new S(7);
  if (allocs != 2 || live != 4) return 6;
  delete[] p;
  delete q;
  if (allocs != 0 || live != 0) return 7;
  return 0;
}
