// RUN: -std=c++17
// EXPECT_EXIT: 0

// A constexpr member returning `this` (or a base subobject of it) for a
// namespace-scope object is an address constant, also when the constexpr
// variable is `const T* const` and when the base is not at offset 0.
struct A {
  int a = 1;
};
struct B {
  int b = 2;
};
struct D : A, B {
  constexpr const D* self() const { return this; }
  constexpr const A* as_a() const { return this; }
  constexpr const B* as_b() const { return this; }
};

const D d{};
constexpr const D* self = d.self();
constexpr const A* pa = d.as_a();
constexpr const B* pb = d.as_b();

const int numbers[3] = {1, 2, 3};
constexpr const int* first = numbers;
constexpr const int* last = &numbers[2];

int main() {
  if (self != &d) return 1;
  if (pa != static_cast<const A*>(&d) || pa->a != 1) return 2;
  if (pb != static_cast<const B*>(&d) || pb->b != 2) return 3;
  if (first != &numbers[0] || *last != 3) return 4;
  return 0;
}
