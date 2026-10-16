// std::reference_wrapper binds only lvalues: its converting constructor is
// constrained so that `const int&` from an rvalue int is rejected, and
// ref/cref of a reference_wrapper unwrap instead of nesting.
#include <functional>
#include <type_traits>

struct Base {
  int v = 1;
};
struct Derived : Base {};

struct ToInt {
  int& target;
  operator int&() const { return target; }
};

static_assert(std::is_convertible<const int&, std::reference_wrapper<const int>>::value, "");
static_assert(std::is_convertible<int&, std::reference_wrapper<const int>>::value, "");
static_assert(!std::is_convertible<int, std::reference_wrapper<const int>>::value, "");
static_assert(!std::is_constructible<std::reference_wrapper<const int>, int&&>::value, "");
static_assert(!std::is_constructible<std::reference_wrapper<int>, const int&>::value, "");
static_assert(std::is_convertible<Derived&, std::reference_wrapper<Base>>::value, "");
static_assert(std::is_convertible<ToInt, std::reference_wrapper<int>>::value, "");
static_assert(std::is_copy_constructible<std::reference_wrapper<int>>::value, "");
static_assert(std::is_copy_assignable<std::reference_wrapper<int>>::value, "");
static_assert(std::is_trivially_copyable<std::reference_wrapper<int>>::value, "");

int main() {
  int a = 3, b = 4;
  std::reference_wrapper<int> r = a;
  std::reference_wrapper<int> copy = r;
  if (&copy.get() != &a) return 1;
  copy = std::ref(b);
  if (&copy.get() != &b || &r.get() != &a) return 2;

  auto twice = std::ref(r);
  static_assert(std::is_same<decltype(twice), std::reference_wrapper<int>>::value, "");
  if (&twice.get() != &a) return 3;
  auto c = std::cref(r);
  static_assert(std::is_same<decltype(c), std::reference_wrapper<const int>>::value, "");
  if (&c.get() != &a) return 4;

  std::reference_wrapper deduced = b;
  static_assert(std::is_same<decltype(deduced), std::reference_wrapper<int>>::value, "");
  int& via_conversion = deduced;
  if (&via_conversion != &b) return 5;

  Derived d;
  std::reference_wrapper<Base> base = d;
  if (&base.get() != static_cast<Base*>(&d)) return 6;

  ToInt t{a};
  std::reference_wrapper<int> from_conversion = t;
  if (&from_conversion.get() != &a) return 7;

  auto add = [](int x, int y) { return x + y; };
  if (std::ref(add)(a, b) != 7) return 8;
  return 0;
}
