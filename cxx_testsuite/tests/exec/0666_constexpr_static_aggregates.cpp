// RUN: -std=c++20
// EXPECT_EXIT: 0

// Constexpr objects emitted as static data: omitted array members and
// elements, default-initialized classes and arrays of them, and
// std::string_view objects that point into string literals.

#include <string_view>

struct Inner {
  int p;
  int q = 7;
};

struct Outer {
  Inner in;
  int z;
  int arr[2];
};

constexpr Outer omitted_array = {{1}, 2};
constexpr Outer elided = {1, 2, 3, 4};

struct Plain {
  int p;
  int q;
};

constexpr Plain plains[2] = {{1, 2}};
constexpr Inner inners[3] = {{8}};
constexpr Inner unsized[] = {{8}};

struct Ctor {
  int x;
  constexpr Ctor() : x(3) {}
};

constexpr Ctor ctor;
constexpr Ctor ctors[2];
constexpr Ctor from_empty[2] = {};
constinit Ctor constinit_ctor;

constexpr std::string_view abc("abc");
constexpr std::string_view tail = abc.substr(1);
constexpr std::string_view none;

int main() {
  const Outer* o = &omitted_array;
  if (o->in.p != 1 || o->in.q != 7 || o->z != 2 || o->arr[0] != 0 ||
      o->arr[1] != 0) {
    return 1;
  }
  const Outer* e = &elided;
  if (e->in.q != 2 || e->z != 3 || e->arr[0] != 4 || e->arr[1] != 0) {
    return 2;
  }
  const Plain* p = plains;
  if (p[0].q != 2 || p[1].p != 0 || p[1].q != 0) {
    return 3;
  }
  const Inner* in = inners;
  if (in[0].p != 8 || in[0].q != 7 || in[2].q != 7) {
    return 4;
  }
  if (sizeof(unsized) != sizeof(Inner) || unsized[0].q != 7) {
    return 5;
  }
  const Ctor* c = ctors;
  if (ctor.x != 3 || c[1].x != 3 || from_empty[1].x != 3 ||
      constinit_ctor.x != 3) {
    return 6;
  }
  std::string_view t = tail;
  if (t.size() != 2 || t[0] != 'b' || t != "bc" || abc.compare("abc") != 0) {
    return 7;
  }
  if (!none.empty() || none.data() != nullptr) {
    return 8;
  }
  return 0;
}
