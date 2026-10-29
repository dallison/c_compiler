// RUN: -std=c++20
// EXPECT_EXIT: 0

// `if constexpr` inside a generic lambda of a class-template member must
// discard the branch that is ill-formed for this T.  The condition names the
// enclosing class parameter (`std::is_pointer<T>::value`).

#include <tuple>
#include <type_traits>

struct S {
  int n;
  S& operator=(int v) {
    n = v;
    return *this;
  }
};

template <class T>
struct Hold {
  T obj;
  int* p;

  void run_empty() {
    std::apply(
        [this](auto&&...) {
          if constexpr (std::is_pointer<T>::value) {
            obj = static_cast<T>(p);
          } else {
            obj = 3;
          }
        },
        std::tuple<>{});
  }

  int run_arg() {
    return std::apply(
        [this](auto&& x) {
          if constexpr (std::is_pointer<T>::value) {
            obj = static_cast<T>(p);
          } else {
            obj = 3;
          }
          return static_cast<int>(x);
        },
        std::tuple<int>{7});
  }
};

int main() {
  Hold<S> hold;
  hold.obj.n = 0;
  hold.p = 0;
  hold.run_empty();
  if (hold.obj.n != 3) {
    return 2;
  }
  hold.obj.n = 0;
  if (hold.run_arg() != 7 || hold.obj.n != 3) {
    return 3;
  }

  int slot = 0;
  Hold<int*> ptrs;
  ptrs.obj = 0;
  ptrs.p = &slot;
  ptrs.run_empty();
  if (ptrs.obj != &slot) {
    return 4;
  }
  ptrs.obj = 0;
  if (ptrs.run_arg() != 7 || ptrs.obj != &slot) {
    return 5;
  }
  return 0;
}
