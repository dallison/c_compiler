// RUN: -std=c++20
// EXPECT_EXIT: 0

struct A {
  int x;
  int y;
};

struct S {
  int a;
  int A::*m;
};

struct Nested : S {
  S inner[2];
  int A::*tail;
};

union U {
  int A::*m;
  long other;
};

struct WithDefault {
  int A::*with = &A::y;
  int A::*without;
};

struct WithConstructor {
  WithConstructor() : a(1) {}
  int a;
  int A::*m;
  S inner;
};

int A::*Pick(bool y) { return y ? &A::y : &A::x; }

int A::*global_mp;
S global_s;
WithConstructor global_constructed;
WithDefault global_default;
int A::* const global_dynamic = Pick(true);
thread_local S tls_s;
S global_array[3];
Nested global_nested;
U global_union;
int (A::*global_fn)();

template <class T>
struct Holder {
  static T value;
};
template <class T>
T Holder<T>::value;

int CheckNested(const Nested& n) {
  return n.m == nullptr && n.inner[0].m == nullptr &&
                 n.inner[1].m == nullptr && n.tail == nullptr
             ? 0
             : 1;
}

int main() {
  int fails = 0;
  int A::*braced{};
  if (braced != nullptr) fails |= 1 << 0;
  int A::*eq = {};
  if (eq != nullptr) fails |= 1 << 1;
  S s{};
  if (s.m != nullptr) fails |= 1 << 2;
  S s2{1};
  if (s2.m != nullptr) fails |= 1 << 3;
  if (global_mp != nullptr) fails |= 1 << 4;
  if (global_s.m != nullptr) fails |= 1 << 5;
  static int A::*local_static;
  if (local_static != nullptr) fails |= 1 << 6;
  S* heap = new S();
  if (heap->m != nullptr) fails |= 1 << 7;
  delete heap;
  for (const S& e : global_array) {
    if (e.m != nullptr) fails |= 1 << 8;
  }
  if (CheckNested(global_nested) != 0) fails |= 1 << 9;
  Nested local_nested{};
  if (CheckNested(local_nested) != 0) fails |= 1 << 10;
  if (global_union.m != nullptr) fails |= 1 << 11;
  S partial[3] = {{1, &A::y}};
  if (partial[0].m != &A::y || partial[1].m != nullptr ||
      partial[2].m != nullptr)
    fails |= 1 << 12;
  WithDefault d{};
  if (d.with != &A::y || d.without != nullptr) fails |= 1 << 13;
  static WithDefault local_static_default;
  if (local_static_default.with != &A::y ||
      local_static_default.without != nullptr)
    fails |= 1 << 14;
  if (Holder<int A::*>::value != nullptr || Holder<S>::value.m != nullptr)
    fails |= 1 << 15;
  if (global_fn != nullptr) fails |= 1 << 16;
  int A::** heap_mp = new int A::*();
  if (*heap_mp != nullptr) fails |= 1 << 17;
  delete heap_mp;
  int A::** heap_array = new int A::*[2]();
  if (heap_array[0] != nullptr || heap_array[1] != nullptr) fails |= 1 << 18;
  delete[] heap_array;
  S* heap_s_array = new S[2]();
  if (heap_s_array[0].m != nullptr || heap_s_array[1].m != nullptr)
    fails |= 1 << 19;
  delete[] heap_s_array;
  int g = 7;
  int** heap_ptr = new int*(&g);
  int* const* heap_const_ptr = new int* const(&g);
  if (**heap_ptr != 7 || **heap_const_ptr != 7) fails |= 1 << 20;
  delete heap_ptr;
  delete heap_const_ptr;
  int A::** heap_braced = new int A::*{&A::y};
  if (*heap_braced != &A::y) fails |= 1 << 21;
  delete heap_braced;
  if (global_constructed.a != 1 || global_constructed.m != nullptr ||
      global_constructed.inner.m != nullptr)
    fails |= 1 << 22;
  static WithConstructor local_constructed;
  if (local_constructed.a != 1 || local_constructed.m != nullptr ||
      local_constructed.inner.m != nullptr)
    fails |= 1 << 23;
  if (global_default.with != &A::y || global_default.without != nullptr)
    fails |= 1 << 24;
  if (global_dynamic != &A::y) fails |= 1 << 25;
  if (tls_s.m != nullptr) fails |= 1 << 26;
  for (int bit = 0; bit < 32; bit++) {
    if ((fails & (1 << bit)) != 0) return bit + 1;
  }
  return 0;
}
