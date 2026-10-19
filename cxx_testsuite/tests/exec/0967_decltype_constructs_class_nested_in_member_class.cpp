// RUN: -std=c++20
// EXPECT_EXIT: 0

// `Rcvr(0)` in the decltype names the pattern's `Forward<S>::State::Rcvr`.
// Instantiating `State` for `Forward<int>` must construct that
// specialization's `Rcvr`, although `State` itself is not a template.

template <class T>
struct Box {
  T value;
};

template <class S, class U>
Box<U> make(S, U) {
  return Box<U>{};
}

template <class S>
struct Forward {
  struct State {
    struct Rcvr {
      int v;
      Rcvr(int x = 5) : v(x) {}
    };
    using Child = decltype(make(*(S*)0, Rcvr(0)));
    Child child;
  };
};

int main() {
  Forward<int>::State s;
  return s.child.value.v == 5 ? 0 : 1;
}
