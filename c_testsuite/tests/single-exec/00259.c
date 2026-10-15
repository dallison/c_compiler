// Designators naming members of anonymous unions and structs that are not the
// first member of the enclosing struct initialize those members.
#include <stdio.h>

struct S {
  int a;
  union {
    int x;
    float y;
  };
  struct {
    short p, q;
  };
  int z;
};

struct S s1 = {.a = 1, .x = 3, .q = 4, .z = 5};
struct S s2 = {1, {2}, {3, 4}, 5};

int main(void) {
  struct S l = {.y = 1.5f, .p = 7};
  if (s1.a != 1 || s1.x != 3 || s1.q != 4 || s1.z != 5 || s1.p != 0) return 1;
  if (s2.a != 1 || s2.x != 2 || s2.p != 3 || s2.q != 4 || s2.z != 5) return 2;
  if (l.y != 1.5f || l.p != 7 || l.q != 0 || l.a != 0) return 3;
  puts("ok");
  return 0;
}
