// RUN: -std=c++26
// EXPECT_EXIT: 0

// A handler naming `derived&` before any throw of `derived` must not leave the
// shared exception type record describing a reference: the thrown object still
// needs its own size, its class-ness and its bases.

struct base {
  int value;
};

struct derived : base {
  int* destroyed;
  constexpr derived(int v, int* d) : base{v}, destroyed(d) {}
  constexpr derived(const derived& other) = default;
  constexpr ~derived() { *destroyed += 1; }
};

constexpr int catch_derived_reference(int* destroyed) {
  try {
    derived local{1, destroyed};
    throw local;
  } catch (const derived& caught) {
    return caught.value;
  }
}

constexpr int catch_as_base(int* destroyed) {
  try {
    throw derived{2, destroyed};
  } catch (const base& caught) {
    return caught.value;
  }
}

constexpr int run() {
  int destroyed = 0;
  int first = catch_derived_reference(&destroyed);
  if (destroyed != 2) return 100 + destroyed;
  int second = catch_as_base(&destroyed);
  return first * 10 + second;
}

static_assert(run() == 12);

int main() {
  int destroyed = 0;
  if (catch_derived_reference(&destroyed) != 1 || destroyed != 2) return 1;
  if (catch_as_base(&destroyed) != 2) return 2;
  return run() == 12 ? 0 : 3;
}
