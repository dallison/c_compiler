// RUN: -std=c++20
namespace enclosing_to_inline {
struct Widget {
  int v;
};
inline namespace detail {
int grab(Widget w) {
  return w.v + 1;
}
}  // namespace detail
}  // namespace enclosing_to_inline

namespace inline_to_enclosing {
inline namespace detail {
struct Widget {
  int v;
};
}  // namespace detail
int grab(inline_to_enclosing::Widget w) {
  return w.v + 2;
}
}  // namespace inline_to_enclosing

namespace transitive {
inline namespace v1 {
inline namespace v2 {
int value = 10;
}
}  // namespace v1
}  // namespace transitive

namespace anon_inline {
inline namespace {
int hidden = 20;
}
}  // namespace anon_inline

int use_enclosing_to_inline(enclosing_to_inline::Widget w) {
  return grab(w);
}

int use_inline_to_enclosing(inline_to_enclosing::Widget w) {
  return grab(w);
}

int main(void) {
  if (use_enclosing_to_inline(enclosing_to_inline::Widget{3}) != 4) {
    return 1;
  }
  if (use_inline_to_enclosing(inline_to_enclosing::Widget{5}) != 7) {
    return 2;
  }
  if (transitive::value != 10) {
    return 3;
  }
  if (anon_inline::hidden != 20) {
    return 4;
  }
  return 0;
}
