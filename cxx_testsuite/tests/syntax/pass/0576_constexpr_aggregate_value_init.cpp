// RUN: -std=c++20 -fconstexpr-eval=audit

// Constexpr objects whose initializers leave subobjects out or are omitted
// entirely: brace elision into a member with a default member initializer,
// omitted array members and elements, default-initialized class objects and
// arrays, arrays of classes with constexpr default constructors, and unsized
// arrays of aggregates with default member initializers.

struct Inner {
  int p;
  int q = 7;
};

struct Outer {
  Inner in;
  int z;
  int arr[2];
};

// Brace elision consumes `2` for `in.q`, overriding its default.
constexpr Outer elided = {1, 2, 3, 4};
static_assert(elided.in.p == 1 && elided.in.q == 2);
static_assert(elided.z == 3 && elided.arr[0] == 4 && elided.arr[1] == 0);

struct Plain {
  int p;
  int q;
};

struct Outer2 {
  Plain in;
  int z;
};

constexpr Outer2 elided_plain = {1, 2, 3};
static_assert(elided_plain.in.q == 2 && elided_plain.z == 3);

// Omitted array members are value-initialized.
constexpr Outer omitted_array = {{1}, 2};
static_assert(omitted_array.in.q == 7 && omitted_array.z == 2);
static_assert(omitted_array.arr[0] == 0 && omitted_array.arr[1] == 0);

constexpr Outer empty = {};
static_assert(empty.in.q == 7 && empty.arr[1] == 0);

struct ArrayFirst {
  int a;
  int arr[2];
  Inner in;
};

constexpr ArrayFirst array_first = {1};
static_assert(array_first.arr[1] == 0 && array_first.in.q == 7);

// Omitted elements of an array of aggregates.
constexpr Plain plains[2] = {{1, 2}};
static_assert(plains[0].q == 2 && plains[1].p == 0 && plains[1].q == 0);

constexpr Inner inners[3] = {{8}};
static_assert(inners[0].q == 7 && inners[2].p == 0 && inners[2].q == 7);

// An unsized array takes its bound from the initializers alone.
constexpr Inner unsized[] = {{8}};
static_assert(sizeof(unsized) == sizeof(Inner));
static_assert(unsized[0].p == 8 && unsized[0].q == 7);

// Default initialization runs a constexpr default constructor.
struct Ctor {
  int x;
  constexpr Ctor() : x(3) {}
};

constexpr Ctor ctor;
static_assert(ctor.x == 3);

struct Defaults {
  int x = 4;
  int y = 5;
};

constexpr Defaults defaults;
static_assert(defaults.x == 4 && defaults.y == 5);

constexpr Ctor ctors[2];
static_assert(ctors[1].x == 3);

constinit Ctor constinit_ctor;

// Omitted and value-initialized elements of an array of non-aggregates are
// default-constructed.
constexpr Ctor from_empty[2] = {};
static_assert(from_empty[1].x == 3);

constexpr Ctor from_one[2] = {Ctor()};
static_assert(from_one[0].x == 3 && from_one[1].x == 3);

constexpr Ctor list_init[2]{};
static_assert(list_init[0].x == 3);

struct HoldsCtor {
  int y;
  Ctor c;
};

constexpr HoldsCtor holds = {1};
static_assert(holds.y == 1 && holds.c.x == 3);

constexpr int local_array() {
  constexpr Ctor local[3];
  return local[2].x;
}
static_assert(local_array() == 3);

template <class T>
constexpr int template_local_array() {
  constexpr T local[2];
  return local[1].x;
}
static_assert(template_local_array<Ctor>() == 3);

// A null pointer member compares equal however each evaluator stores it.
struct Pointer {
  const char* p;
  constexpr Pointer() : p(nullptr) {}
};

constexpr Pointer null_member;
static_assert(null_member.p == nullptr);

int main() { return 0; }
