// RUN: -std=c++20
// Argument-dependent lookup still resolves the innermost-enclosing-namespace
// function and inline (hidden) friends after restricting ADL to the innermost
// associated namespace.
namespace lib {
namespace detail {
struct Widget {
  int v;
  // Hidden friend: only reachable through ADL on a Widget argument.
  friend int weight(Widget w) { return w.v + 1; }
};

int height(Widget w) {
  return w.v + 2;
}
}  // namespace detail
}  // namespace lib

int use(lib::detail::Widget w) {
  return weight(w) + height(w);
}
