// RUN: -std=c++20
// EXPECT_EXIT: 0

// A constructor initializer that names a nested class must construct the
// specialization's nested class.  Naming the primary template's nested class
// instantiates the callee for a different type and leaves the call unresolved.

template <class T>
struct Box {
  T value;
  explicit Box(T object) : value(object) {}
};

struct connect_t {
  template <class S, class R>
  Box<R> operator()(S, R rcvr) const {
    return Box<R>(rcvr);
  }
};

template <class S, class F>
struct Outer {
  template <class R>
  struct State {
    struct Rcvr {
      int value;
      explicit Rcvr(int v) : value(v) {}
    };
    Box<Rcvr> child;
    State() : child(connect_t{}(0, Rcvr(42))) {}
  };
};

template <class S, class F>
struct Forward {
  template <class R>
  struct State {
    struct Rcvr {
      int value;
      explicit Rcvr(State* state) : value(state->value) {}
    };
    int value;
    using Child =
        decltype(connect_t{}(static_cast<S&&>(*(S*)0), Rcvr((State*)0)));
    Child child;
    State(S&& source)
        : value(7),
          child(connect_t{}(static_cast<S&&>(source), Rcvr(this))) {}
  };
};

int main() {
  Outer<int, int>::State<int> state;
  if (state.child.value.value != 42) return 1;
  int source = 0;
  Forward<int, int>::State<int> forwarded(static_cast<int&&>(source));
  if (forwarded.child.value.value != 7) return 2;
  return 0;
}
