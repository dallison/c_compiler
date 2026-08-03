// RUN: -std=c++23

template <class T, class U>
struct is_same {
  static constexpr bool value = false;
};

template <class T>
struct is_same<T, T> {
  static constexpr bool value = true;
};

template <class First, class Second>
struct Base {
  Base(First, Second) {
  }
};

template <class Left, class Right>
struct Derived : Base<Right, Left> {
  using Base<Right, Left>::Base;
};

Derived reordered(1, 'x');
static_assert(is_same<decltype(reordered), Derived<char, int>>::value);

template <class T>
struct GuidedBase {
  GuidedBase(long) {
  }
};

GuidedBase(long) -> GuidedBase<int>;

template <class T>
struct GuidedDerived : GuidedBase<T> {
  using GuidedBase<T>::GuidedBase;
};

GuidedDerived guided(1L);
static_assert(is_same<decltype(guided), GuidedDerived<int>>::value);

template <class T>
struct PreferredBase {
  PreferredBase(T) {
  }
};

PreferredBase(int) -> PreferredBase<int>;

template <class T>
struct PreferredDerived : PreferredBase<T> {
  using PreferredBase<T>::PreferredBase;
};

PreferredDerived(int) -> PreferredDerived<long>;

PreferredDerived preferred(1);
static_assert(is_same<decltype(preferred), PreferredDerived<long>>::value);

template <class T>
concept FourBytes = sizeof(T) == 4;

template <class T>
struct ConstrainedBase {
  ConstrainedBase(long) {
  }
};

template <FourBytes T>
ConstrainedBase(T) -> ConstrainedBase<T>;

template <class T>
struct ConstrainedDerived : ConstrainedBase<T> {
  using ConstrainedBase<T>::ConstrainedBase;
};

ConstrainedDerived constrained(1);
static_assert(
    is_same<decltype(constrained), ConstrainedDerived<int>>::value);

int main(void) {
  return 0;
}
