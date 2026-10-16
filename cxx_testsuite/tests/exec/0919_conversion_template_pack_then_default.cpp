// gmock's ReturnAction: a conversion operator template whose parameter pack
// is followed by a defaulted parameter.  Converting to Once<int()> deduces an
// empty Args; the default's `void` must not become part of the pack.
#include <type_traits>

template <class F> struct Once;
template <class R, class... A> struct Once<R(A...)> {
  int n = sizeof...(A);
};

template <class R> struct Ret {
  R v;
  template <class U, class... Args,
            typename = typename std::enable_if<
                std::is_convertible<R, U>::value>::type>
  operator Once<U(Args...)>() && {
    return Once<U(Args...)>();
  }
};

template <class F> struct Spec {
  int WillOnce(Once<F> once) { return once.n; }
};

int main() {
  Spec<int()> none;
  Spec<int(int)> one;
  Spec<long(int, char)> two;
  if (none.WillOnce(Ret<int>{1}) != 0) return 1;
  if (one.WillOnce(Ret<int>{1}) != 1) return 2;
  if (two.WillOnce(Ret<int>{1}) != 2) return 3;
  Once<int()> direct = Ret<short>{2};
  if (direct.n != 0) return 4;
  return 0;
}
