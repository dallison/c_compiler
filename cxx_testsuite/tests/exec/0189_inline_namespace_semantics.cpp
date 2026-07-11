// RUN: -std=c++20
namespace lib {
inline namespace v1 {
int pick(int x) {
  return x + 1;
}
}

namespace v2 {
int pick(int x) {
  return x + 10;
}
}
}  // namespace lib

namespace adl_outer {
inline namespace adl_inline {
struct Widget {
  int v;
};
int helper(Widget w) {
  return w.v + 1;
}
}  // namespace adl_inline
}  // namespace adl_outer

int call_adl(adl_outer::Widget w) {
  return helper(w);
}

int main(void) {
  if (lib::pick(3) != 4) {
    return 1;
  }
  if (call_adl(adl_outer::Widget{5}) != 6) {
    return 2;
  }
  return 0;
}
