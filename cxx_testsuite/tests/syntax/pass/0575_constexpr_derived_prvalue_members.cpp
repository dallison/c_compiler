// RUN: -std=c++20 -fconstexpr-eval=audit

// Members of class prvalues and constexpr objects with base classes, read
// directly and through bases: from a call, a braced aggregate, an array of
// them, and with empty, multiple, nested and defaulted bases.

struct Base {
  int tag;
};

struct Derived : Base {
  int a;
  int b;
};

constexpr Derived make() {
  Derived d{};
  d.tag = 1;
  d.a = 2;
  d.b = 3;
  return d;
}

static_assert(make().tag == 1);
static_assert(make().a == 2);
static_assert(make().b == 3);

constexpr int through_function() { return make().a * 10 + make().b; }
static_assert(through_function() == 23);

constexpr Derived made = make();
static_assert(made.tag == 1 && made.a == 2 && made.b == 3);

constexpr Derived braced = {{4}, 5, 6};
static_assert(braced.tag == 4 && braced.a == 5 && braced.b == 6);

constexpr Derived omitted_base = {};
static_assert(omitted_base.tag == 0 && omitted_base.a == 0);

constexpr Derived pair[2] = {{{1}, 2, 3}, {{4}, 5, 6}};
static_assert(pair[1].tag == 4 && pair[1].b == 6);

struct Empty {
  constexpr int answer() const { return 42; }
};

struct Other {
  short x;
  short y;
};

struct Many : Empty, Base, Other {
  int c;
};

constexpr Many make_many() { return Many{{}, {7}, {8, 9}, 10}; }
static_assert(make_many().tag == 7);
static_assert(make_many().x == 8 && make_many().y == 9);
static_assert(make_many().c == 10);
static_assert(make_many().answer() == 42);

constexpr Many many = make_many();
static_assert(many.tag == 7 && many.y == 9 && many.c == 10);

struct Nested : Derived {
  int d;
};

constexpr Nested make_nested() {
  Nested n{};
  n.tag = 11;
  n.a = 12;
  n.b = 13;
  n.d = 14;
  return n;
}
static_assert(make_nested().tag == 11 && make_nested().a == 12);
static_assert(make_nested().d == 14);

struct Defaulted {
  int value = 5;
};

struct UsesDefault : Defaulted {
  int own;
};

constexpr UsesDefault uses_default = {{}, 1};
static_assert(uses_default.value == 5 && uses_default.own == 1);

constexpr UsesDefault make_default() { return UsesDefault{{}, 2}; }
static_assert(make_default().value == 5 && make_default().own == 2);

constexpr UsesDefault empty_list = {};
static_assert(empty_list.value == 5 && empty_list.own == 0);

constexpr int local_empty_list() {
  UsesDefault u = {};
  return u.value;
}
static_assert(local_empty_list() == 5);

constexpr Derived elided = {1, 2, 3};
static_assert(elided.tag == 1 && elided.a == 2 && elided.b == 3);

constexpr int local_elided() {
  Derived d = {1, 2, 3};
  return d.tag * 100 + d.a * 10 + d.b;
}
static_assert(local_elided() == 123);

struct TwoBases : Base, Defaulted {
  int z;
};

struct SelfNamed {
  using self = SelfNamed;
  int v = 3;
};

struct HoldsSelfNamed : Base {
  SelfNamed held;
};

constexpr HoldsSelfNamed holds_self_named = {{1}};
static_assert(holds_self_named.held.v == 3);

constexpr TwoBases first_base_only = {{9}};
static_assert(first_base_only.tag == 9 && first_base_only.value == 5 &&
              first_base_only.z == 0);
