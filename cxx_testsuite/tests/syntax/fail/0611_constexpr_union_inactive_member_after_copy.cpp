// RUN: -std=c++20
// EXPECT: constexpr variable initializer is not a constant expression

// A copy of a union keeps the source's active member; reading another member
// is not a constant expression.
union U { int i; float f; };
constexpr int copied() { U a{.f = 1}; U b = a; return b.i; }
constexpr int x = copied();
constexpr int assigned() { U a{.f = 1}; U b{.i = 2}; b = a; return b.i; }
constexpr int y = assigned();
struct A { U arr[2]; constexpr A() : arr{{.f = 1.0f}, {.i = 9}} {} };
constexpr A a;
constexpr int z = a.arr[0].i;

int main() { return 0; }
