// RUN: -std=c++26
// EXPECT: constexpr conversion from void pointer requires an object of similar type

struct base {
  int value;
};

struct derived : base {
  int other;
};

constexpr bool base_subobject_round_trip() {
  derived object = {{1}, 2};
  base* subobject = &object;
  void* erased = subobject;
  base* restored = static_cast<base*>(erased);
  return restored->value == 1;
}

static_assert(base_subobject_round_trip());
