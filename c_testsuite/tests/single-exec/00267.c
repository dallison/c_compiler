#include <stdio.h>

// Strength reducing p[i] in a loop must not leave stale variable annotations
// on loads further along the address chain (p[i]->field).

struct Inner {
  char pad[16];
  unsigned instance;
};

struct Outer {
  char pad[300];
  struct Inner inner;
};

static struct Outer *peers[4];
static int peer_count;

static int HasInstance(unsigned instance) {
  int i;
  for (i = 0; i < peer_count; i++) {
    if (peers[i]->inner.instance == instance) {
      return 1;
    }
  }
  return 0;
}

static void SetInstances(unsigned value) {
  int i;
  for (i = 0; i < peer_count; i++) {
    peers[i]->inner.instance = value + i;
  }
}

int main(void) {
  static struct Outer a, b, c;
  peers[0] = &a;
  peers[1] = &b;
  peers[2] = &c;
  peer_count = 3;
  SetInstances(10);
  if (!HasInstance(10) || !HasInstance(12)) return 1;
  if (HasInstance(13)) return 2;
  puts("ok");
  return 0;
}
