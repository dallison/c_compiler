// RUN: -std=c++23

#include <type_traits>
#include <utility>
#include <version>

#if __cpp_lib_invoke_r != 202106L
#error "__cpp_lib_invoke_r has the wrong value"
#endif

#if __cpp_lib_bind_back != 202202L
#error "__cpp_lib_bind_back has the wrong value"
#endif

#if __cpp_lib_move_only_function != 202110L
#error "__cpp_lib_move_only_function has the wrong value"
#endif

struct callable {
  int operator()(int value) & {
    return value;
  }

  int operator()(int value) && {
    return value;
  }
};

struct holder {
  callable value;
};

static_assert(std::is_same_v<
              decltype((static_cast<holder&&>(std::declval<holder&>()).value)),
              callable&&>);

template <class Function, class First, class... Rest>
int forward_pack(Function&& function, First&& first, Rest&&... rest) {
  return static_cast<Function&&>(function)(
      static_cast<First&&>(first), static_cast<Rest&&>(rest)...);
}

struct binary_callable {
  int operator()(int, int) const&& {
    return 0;
  }
};

inline int test_const_rvalue_pack() {
  int value = 0;
  return forward_pack(static_cast<const binary_callable&&>(binary_callable{}),
                      1, static_cast<const int&&>(value));
}

template <int Mode, class Function>
struct mode_dispatch {
  static int call(Function& function, int value) {
    if constexpr (Mode == 1) {
      return static_cast<const Function&>(function)(value);
    } else if constexpr (Mode == 2) {
      return static_cast<const Function&&>(function)(value);
    } else {
      return function(value);
    }
  }
};

inline int test_discarded_constexpr_branches() {
  callable function;
  return mode_dispatch<0, callable>::call(function, 1);
}
