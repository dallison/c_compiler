// RUN: -std=c++20
// EXPECT_EXIT: 0

template <class Derived>
struct Interface {
  Derived* derived() { return static_cast<Derived*>(this); }
  const Derived* derived() const { return static_cast<const Derived*>(this); }

  int value() { return derived()->value_impl(); }
  int overloaded(int x) { return derived()->overload_impl(x); }
  int overloaded(long x) { return derived()->overload_impl(x); }
  int const_value() const { return derived()->const_value_impl(); }
};

template <class Derived>
struct DependentBase {
  int base_value() { return 7; }
};

template <class Derived>
struct UsesDependentBase : DependentBase<Derived> {
  int via_this() { return this->base_value(); }
  int via_qualified() { return DependentBase<Derived>::base_value(); }
};

struct Widget : Interface<Widget>, UsesDependentBase<Widget> {
  int value_impl() { return 11; }
  int overload_impl(int) { return 13; }
  int overload_impl(long) { return 17; }
  int const_value_impl() const { return 19; }
};

int main() {
  Widget widget;
  if (widget.value() != 11) {
    return 1;
  }
  if (widget.overloaded(1) != 13) {
    return 2;
  }
  if (widget.overloaded(1L) != 17) {
    return 3;
  }
  const Widget const_widget;
  if (const_widget.const_value() != 19) {
    return 4;
  }
  if (widget.via_this() != 7) {
    return 5;
  }
  if (widget.via_qualified() != 7) {
    return 6;
  }
  return 0;
}
