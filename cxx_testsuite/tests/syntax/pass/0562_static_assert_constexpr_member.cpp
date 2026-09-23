// RUN: -std=c++17
//
// static_assert inside a class can call a constexpr static member function
// whose body is parsed only after the class is complete.  Inside a class
// template the operand may also depend on a static member initialized from
// a template argument, so the check is repeated at instantiation.

struct Holder {
  static constexpr int id(int x) { return x; }
  static constexpr bool ok(int x) { return x == 3; }
  static constexpr int value = 3;
  static_assert(id(3) == 3, "id");
  static_assert(ok(value), "ok");
};

static_assert(Holder::id(4) == 4, "out");

template <class T>
struct Box {
  static constexpr int id(int x) { return x; }
  static constexpr int value = T::n;
  static_assert(id(value) == 7, "box");
};

struct Seven {
  static constexpr int n = 7;
};

static_assert(Box<Seven>::value == 7, "boxed");
