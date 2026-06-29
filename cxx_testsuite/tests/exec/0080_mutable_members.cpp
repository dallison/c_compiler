// RUN: -std=c++20
// EXPECT_EXIT: 0
//
// A 'mutable' data member may be modified through a const object or from a
// const-qualified member function, while ordinary members may not.

struct Cache {
  int value;
  mutable int hits;

  int read(void) const {
    hits = hits + 1;  // OK: hits is mutable even though read() is const.
    return value;
  }
};

static int peek(const Cache& c) {
  c.hits = c.hits + 10;  // OK: mutable member through a const reference.
  return c.hits;
}

int main(void) {
  Cache cache;
  cache.value = 42;
  cache.hits = 0;

  if (cache.read() != 42) {
    return 1;
  }
  if (cache.hits != 1) {
    return 2;
  }
  if (peek(cache) != 11) {
    return 3;
  }
  if (cache.read() != 42 || cache.hits != 12) {
    return 4;
  }
  return 0;
}
