// [over.match.funcs]/9: a constructor inherited from C whose first parameter
// is a reference to C is not a candidate for a single argument, so wrappers
// that inherit their comparator's constructors still convert through their
// own by-value constructors (the absl::btree checked comparator pattern).
#include <utility>

template <class T> struct Alloc { Alloc() = default; };

template <class C> struct CheckedBase : C {
  using C::C;
  explicit CheckedBase(C c) : C(std::move(c)) {}
};

template <class C> struct Checked : CheckedBase<C> {
  using Base = CheckedBase<C>;
  using Base::Base;
  Checked(C cmp) : Base(std::move(cmp)) {}
};

template <class C> struct Container {
  using key_compare = Checked<C>;
  explicit Container(const key_compare& comp, const Alloc<int>& = Alloc<int>())
      : cmp(comp) {}
  key_compare cmp;
};

template <class C> struct Set : Container<C> {
  using Base = Container<C>;
  using Base::Base;
};

struct Cmp {
  const int& index;
  bool operator()(int a, int b) const { return a < b; }
};

struct Point {
  int x, y;
  Point(int x, int y) : x(x), y(y) {}
};

struct Derived : Point {
  using Point::Point;
};

int main() {
  int x = 0;
  Set<Cmp> set{Cmp{x}};
  if (&set.cmp.index != &x) return 1;
  if (!set.cmp(1, 2) || set.cmp(2, 1)) return 2;

  Checked<Cmp> direct(Cmp{x});
  if (&direct.index != &x) return 3;

  Derived d(3, 4);
  if (d.x != 3 || d.y != 4) return 4;
  return 0;
}
