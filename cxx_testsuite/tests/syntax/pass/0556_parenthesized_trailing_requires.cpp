// RUN: -std=c++20

template <class T>
concept integral = true;

template <class S, class I>
concept sized_sentinel_for = true;

template <class W, class Bound>
struct iota_view {
  constexpr auto size() const
    requires(integral<W> && integral<Bound>) || sized_sentinel_for<Bound, W>
  {
    return 0;
  }

  constexpr auto empty() const
    requires(integral<W>)
  {
    return false;
  }
};

template <class T>
constexpr bool always_true_v = true;

template <class T>
struct VarReq {
  constexpr auto f() const
    requires always_true_v<T>
  {
    return 1;
  }
};

int main() {
  iota_view<int, int> v;
  return v.size() + (v.empty() ? 1 : 0) + VarReq<int>().f() - 1;
}
