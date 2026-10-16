// A conversion function whose result is a reference, or a scalar, reaches
// non-class reference and value targets ([dcl.init.ref], [over.match.ref]):
// `operator int&` binds `int&` and `const int&` directly, converts to `long`,
// and initializes the temporary for `const long&`, but never binds `int&&`;
// a prvalue result binds only `const int&` or `int&&`.  Overload resolution
// ranks these by the trailing standard conversion and binding.
#include <type_traits>
#include <utility>

struct ToInt {
  int& target;
  operator int&() const { return target; }
};
struct Value {
  operator int() const { return 9; }
};
struct ConstRef {
  const int& target;
  operator const int&() const { return target; }
};

int by_ref(int& v) { return v; }
int by_const_ref(const int& v) { return v; }
int by_long(long v) { return (int)v; }
long by_const_long(const long& v) { return v; }
int by_rref(int&& v) { return v; }
int pick(int) { return 1; }
int pick(long) { return 2; }
int pick_ref(int&) { return 1; }
int pick_ref(const int&) { return 2; }
template <class T> int deduced(T&) { return 1; }
template <class T> int deduced(T&&) { return 2; }

template <class T> void bind(T&) noexcept;
template <class T> void bind(T&&) = delete;
template <class U, class = decltype(bind<int>(std::declval<U>()))>
constexpr bool binds_lvalue(int) { return true; }
template <class U>
constexpr bool binds_lvalue(...) { return false; }

static_assert(std::is_convertible<ToInt, int&>::value, "");
static_assert(std::is_convertible<ToInt, const int&>::value, "");
static_assert(!std::is_convertible<ToInt, int&&>::value, "");
static_assert(std::is_convertible<ToInt, long>::value, "");
static_assert(std::is_convertible<ToInt, const long&>::value, "");
static_assert(!std::is_convertible<Value, int&>::value, "");
static_assert(std::is_convertible<Value, const int&>::value, "");
static_assert(std::is_convertible<Value, int&&>::value, "");
static_assert(!std::is_convertible<ConstRef, int&>::value, "");
static_assert(std::is_convertible<ConstRef, const int&>::value, "");
static_assert(binds_lvalue<ToInt>(0), "");
static_assert(!binds_lvalue<Value>(0), "");

int main() {
  int a = 5;
  ToInt t{a};
  if (by_ref(t) != 5 || by_ref(ToInt{a}) != 5) return 1;
  if (by_const_ref(t) != 5) return 2;
  if (by_long(t) != 5) return 3;
  if (by_const_long(t) != 5) return 4;
  int& direct = t;
  const int& const_direct = t;
  if (&direct != &a || &const_direct != &a) return 5;
  const long& temporary = t;
  a = 6;
  if (temporary != 5 || direct != 6) return 6;

  Value v;
  const int& from_value = v;
  if (from_value != 9 || by_rref(v) != 9) return 7;
  if (pick(v) != 1 || pick(t) != 1) return 8;
  if (pick_ref(t) != 1) return 9;
  ConstRef c{a};
  if (pick_ref(c) != 2) return 10;
  const int& from_const = c;
  if (&from_const != &a) return 11;
  if (deduced<int>(t) != 1 || deduced<int>(ToInt{a}) != 1) return 12;
  return 0;
}
