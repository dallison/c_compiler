// RUN: -std=c++20
// EXPECT_EXIT: 0

// Namespace-scope arrays of class type in each initializer form: default
// construction, converting constructors, an unknown bound counted through
// brace elision into array elements, thread_local arrays, a static data
// member array, and a dynamically initialized array of unknown bound.

#include <string>

int constructed;
int Four();

int dyn[] = {Four(), Four(), Four()};
static_assert(sizeof(dyn) == 3 * sizeof(int));

struct V {
  int x;
  V(int v = 7) : x(v) { constructed++; }
};

V defaulted[3];
V converted[2] = {1, 2};
V converted_unknown[] = {1, 2, 3, 4};
static_assert(sizeof(converted_unknown) == 4 * sizeof(V));
V elided[][2] = {V(1), V(2), V(3)};
static_assert(sizeof(elided) == 4 * sizeof(V));
V mixed[][2] = {{V(1)}, V(2), V(3), {V(4)}};
static_assert(sizeof(mixed) == 6 * sizeof(V));
thread_local V tls_defaulted[3];
thread_local V tls_unknown[] = {V(Four()), V(2)};
static_assert(sizeof(tls_unknown) == 2 * sizeof(V));
std::string strings[] = {"a", std::string("bb"), "ccc"};
static_assert(sizeof(strings) == 3 * sizeof(std::string));

struct S {
  static V member[2];
};
V S::member[2];

int Four() { return 4; }

int main() {
  if (dyn[2] != 4) {
    return 1;
  }
  if (defaulted[2].x != 7 || converted[1].x != 2 ||
      converted_unknown[3].x != 4) {
    return 2;
  }
  if (elided[1][0].x != 3 || elided[1][1].x != 7) {
    return 3;
  }
  if (mixed[0][1].x != 7 || mixed[1][0].x != 2 || mixed[1][1].x != 3 ||
      mixed[2][0].x != 4) {
    return 4;
  }
  if (tls_defaulted[2].x != 7 || tls_unknown[0].x != 4) {
    return 5;
  }
  if (strings[1] != "bb" || strings[2].size() != 3) {
    return 6;
  }
  if (S::member[1].x != 7) {
    return 7;
  }
  // 3 + 2 + 4 + 4 + 6 namespace-scope elements, 2 static members, and the
  // 5 thread_local elements of the main thread.
  if (constructed != 26) {
    return 8;
  }
  return 0;
}
