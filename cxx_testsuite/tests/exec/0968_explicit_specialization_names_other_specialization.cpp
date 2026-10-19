// RUN: -std=c++20
// EXPECT_EXIT: 0

// Inside the explicit specialization H<N>, `H` is the injected-class-name of
// H<N>, but `H<void>` and `H<int>` name other specializations of the primary
// (std::coroutine_handle<noop_coroutine_promise> converts to
// coroutine_handle<void> this way).

template <class P = void>
struct H {
  void* h;
  int primary = 1;
};

struct N {};

template <>
struct H<N> {
  H<N>* self() { return this; }
  H<void> to_void() const {
    H<void> r;
    r.h = h;
    return r;
  }
  operator H<int>() const {
    H<int> r;
    r.h = h;
    return r;
  }
  void* h;
};

int main() {
  int x = 0;
  H<N> n{&x};
  H<void> v = n.to_void();
  H<int> i = n;
  if (n.self() != &n) return 1;
  if (v.h != &x || v.primary != 1) return 2;
  if (i.h != &x || i.primary != 1) return 3;
  return 0;
}
