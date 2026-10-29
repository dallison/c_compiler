// RUN: -std=c++20
// EXPECT_EXIT: 0

// `if constexpr` inside a generic lambda of a class-template member function
// template.  The condition names the class parameter.  A use of the member
// template parameter is an odr-use only in the branch that is kept, and an
// unevaluated operand does not capture it.

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

  template <class U>
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

  template <class U>
  int run_captured(U x) {
    return std::apply(
        [this, x](auto&&...) {
          if constexpr (std::is_pointer<T>::value) {
            obj = static_cast<T>(p);
          } else {
            obj = static_cast<int>(x);
          }
          return static_cast<int>(x);
        },
        std::tuple<>{});
  }

  template <class U>
  int run_unevaluated(U x) {
    return std::apply(
        [this](auto&&...) {
          if constexpr (std::is_pointer<T>::value) {
            obj = static_cast<T>(p);
            return 1;
          } else {
            obj = 3;
            return (int)sizeof(x);
          }
        },
        std::tuple<>{});
  }
};

int discarded(int x) {
  auto lambda = [](auto&&...) {
    if constexpr (sizeof(int) == 1) {
      return x;
    } else {
      return 5;
    }
  };
  return lambda();
}

int sized(int x) {
  auto lambda = [](auto&&...) { return (int)sizeof(x); };
  return lambda();
}

int main() {
  if (discarded(9) != 5 || sized(9) != (int)sizeof(int)) {
    return 2;
  }

  Hold<S> hold;
  hold.obj.n = 0;
  hold.p = 0;
  hold.run_empty<char>();
  if (hold.obj.n != 3) {
    return 3;
  }
  hold.obj.n = 0;
  if (hold.run_captured(7) != 7 || hold.obj.n != 7) {
    return 4;
  }
  hold.obj.n = 0;
  if (hold.run_unevaluated(0) != (int)sizeof(int) || hold.obj.n != 3) {
    return 5;
  }

  int slot = 0;
  Hold<int*> ptrs;
  ptrs.obj = 0;
  ptrs.p = &slot;
  ptrs.run_empty<char>();
  if (ptrs.obj != &slot) {
    return 6;
  }
  ptrs.obj = 0;
  if (ptrs.run_captured(7) != 7 || ptrs.obj != &slot) {
    return 7;
  }
  ptrs.obj = 0;
  if (ptrs.run_unevaluated(0) != 1 || ptrs.obj != &slot) {
    return 8;
  }
  return 0;
}
