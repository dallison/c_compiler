// RUN: -std=c++20
// EXPECT_EXIT: 0

// Instantiating `F<int(long)>` must not renumber the template parameters in
// the primary's mem-initializer casts: `F<int(int)>` would then cast `args`
// to `T` (`lt`) instead of `CA`.
#include <initializer_list>

template <class T>
struct ipt {
  explicit ipt() = default;
};
template <class T>
inline constexpr ipt<T> ip{};

struct lt {
  int total;
  lt(std::initializer_list<int> values, int extra) : total(extra) {
    for (int value : values) {
      total += value;
    }
  }
};

template <class S>
struct B;
template <class R, class... A>
struct B<R(A...)> {
  void* p;
  template <class T, class... CA>
  explicit B(ipt<T>, CA&&... args) {
    p = new T(static_cast<CA&&>(args)...);
  }
  template <class T, class U, class... CA>
  explicit B(ipt<T>, std::initializer_list<U> v, CA&&... args) {
    p = new T(v, static_cast<CA&&>(args)...);
  }
};

template <class S>
struct F;
template <class R, class... A>
struct F<R(A...)> : B<R(A...)> {
  using base = B<R(A...)>;
  template <class T, class... CA>
  explicit F(ipt<T>, CA&&... args) : base(ip<T>, static_cast<CA&&>(args)...) {}
  template <class T, class U, class... CA>
  explicit F(ipt<T>, std::initializer_list<U> v, CA&&... args)
      : base(ip<T>, v, static_cast<CA&&>(args)...) {}
};

static_assert(sizeof(F<int(long)>) > 0);

int main() {
  F<int(int)> b(ip<lt>, {1, 2, 3}, 4);
  lt* made = static_cast<lt*>(b.p);
  int total = made->total;
  delete made;
  return total == 10 ? 0 : 1;
}
