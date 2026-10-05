// RUN: -std=c++26

// A pointer to a base class subobject that round-trips through void* still
// points to an object of the base type, so the conversion back is a constant
// expression.

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
