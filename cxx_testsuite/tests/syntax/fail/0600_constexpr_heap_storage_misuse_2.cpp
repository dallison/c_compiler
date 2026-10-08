// RUN: -std=c++20
// EXPECT: 0600_constexpr_heap_storage_misuse_2.cpp:10: constexpr variable initializer is not a constant expression
// EXPECT: 0600_constexpr_heap_storage_misuse_2.cpp:13: constexpr variable initializer is not a constant expression
// EXPECT: 0600_constexpr_heap_storage_misuse_2.cpp:16: constexpr variable initializer is not a constant expression

// Reads of allocated scalar storage in constant evaluation ([expr.const]):
// past the end, after delete and before initialization are ill-formed.

constexpr int PastEnd() { int* p = new int[2]{1, 2}; int v = p[2]; delete[] p; return v; }
constexpr int r1 = PastEnd();

constexpr int UseAfterDelete() { int* p = new int(4); delete p; return *p; }
constexpr int r2 = UseAfterDelete();

constexpr int Uninitialized() { int* p = new int; int v = *p; delete p; return v; }
constexpr int r3 = Uninitialized();

int main() { return 0; }
