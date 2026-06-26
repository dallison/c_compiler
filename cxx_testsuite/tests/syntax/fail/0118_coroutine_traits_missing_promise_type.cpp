// RUN: -std=c++20
// EXPECT: coroutine return type must provide promise_type

struct Task {
  int value;
};

namespace std {
template <class R, class Arg>
struct coroutine_traits {
};
}

Task coroutine_traits_missing_promise_type(int input) {
  co_return input;
}
