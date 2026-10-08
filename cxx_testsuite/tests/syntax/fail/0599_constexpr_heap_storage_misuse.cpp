// RUN: -std=c++20
// EXPECT: 0599_constexpr_heap_storage_misuse.cpp:15: constexpr variable initializer is not a constant expression
// EXPECT: 0599_constexpr_heap_storage_misuse.cpp:18: constexpr variable initializer is not a constant expression
// EXPECT: 0599_constexpr_heap_storage_misuse.cpp:22: constexpr variable initializer is not a constant expression

// Misuse of allocated scalar storage in constant evaluation ([expr.const]):
// deleting an interior pointer, a pointer to a local or the same storage
// twice.  0600 has more cases; together they would exceed the error limit.

constexpr int DeleteInterior() {
  int* p = new int[2]{1, 2};
  delete (p + 1);
  return 1;
}
constexpr int r1 = DeleteInterior();

constexpr int DeleteLocal() { int a = 1; int* p = &a; delete p; return 1; }
constexpr int r2 = DeleteLocal();

constexpr int DoubleDelete() {
  int* p = new int(1); delete p; delete p; return 1; }
constexpr int r3 = DoubleDelete();

int main() { return 0; }
