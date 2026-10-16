// RUN: -std=c++20
// EXPECT_EXIT: 0

// [conv.bool]: pointers to data members and member functions (virtual or
// not) convert to bool as "not null" in conditions, `!`, `&&`, `?:`,
// initialization, static_cast and static_assert, at run time and in constant
// expressions, including a constexpr null data member pointer whose encoding
// is not zero.
struct Foo {
  int n;
  int m;
  void f() {}
  virtual void g() {}
};

constexpr int Foo::*data_set = &Foo::m;
constexpr int Foo::*data_null = nullptr;
static_assert(data_set);
static_assert(!data_null);
static_assert(data_null == nullptr);
static_assert(static_cast<bool>(&Foo::n));
static_assert(!static_cast<int Foo::*>(nullptr));
static_assert(&Foo::f);
static_assert(&Foo::g);

struct Holder {
  static constexpr int Foo::*member = &Foo::m;
  static_assert(member);
};

template <class T>
bool truthy(T p) {
  return p ? true : false;
}

template <class T>
bool negate(T p) {
  return !p;
}

int main() {
  void (Foo::*function)() = nullptr;
  int Foo::*data = nullptr;
  int result = 0;
  if (function) result |= 1;
  if (!function) result |= 2;
  bool b = data;
  if (b) result |= 4;
  data = &Foo::n;
  if (data && !!data) result |= 8;
  function = &Foo::f;
  bool c = function;
  if (c && (function ? true : false)) result |= 16;
  while (!function) {
  }
  void (Foo::*virtual_function)() = &Foo::g;
  if (truthy(virtual_function) && negate(static_cast<int Foo::*>(nullptr)) &&
      truthy(data) && !static_cast<bool>(static_cast<void (Foo::*)()>(nullptr)))
    result |= 32;
  constexpr int Foo::*local = &Foo::n;
  static_assert(local);
  return result == (2 | 8 | 16 | 32) ? 0 : 100 + result;
}
