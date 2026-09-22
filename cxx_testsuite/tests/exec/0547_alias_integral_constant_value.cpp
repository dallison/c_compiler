// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <type_traits>

template <class T>
using AlwaysTrue = std::integral_constant<bool, true>;

template <class... T>
using TrueAlias =
    std::integral_constant<bool, sizeof(typename std::void_t<T...>*) != 0>;

template <class T>
using SizedTrue = std::integral_constant<bool, sizeof(T*) != 0>;

template <class T, typename = std::enable_if_t<AlwaysTrue<T>::value>>
constexpr int accept_always(T) {
  return 1;
}

template <class T, typename = std::enable_if_t<TrueAlias<T>::value>>
constexpr int accept_true_alias(T) {
  return 2;
}

template <class T, typename = std::enable_if_t<SizedTrue<T>::value>>
constexpr int accept_sized(T) {
  return 3;
}

template <class Sig, class F,
          class = std::enable_if_t<!std::is_same<F, Sig>::value>>
using CanConvert = TrueAlias<
    std::enable_if_t<AlwaysTrue<F>::value>>;

template <class Sig, class F>
using CanAssignReferenceWrapper = TrueAlias<
    std::enable_if_t<AlwaysTrue<F>::value>,
    std::enable_if_t<SizedTrue<F>::value>>;

template <class Sig>
class AnyInvocable {
  template <class F, typename = std::enable_if_t<CanConvert<Sig, F>::value>>
  AnyInvocable(F&& f) {
    (void)f;
  }

  template <class F, typename = std::enable_if_t<
                         CanAssignReferenceWrapper<Sig, F>::value>>
  AnyInvocable& operator=(F&& f) {
    (void)f;
    return *this;
  }
};

int main() {
  if (!AlwaysTrue<char>::value) {
    return 1;
  }
  if (!TrueAlias<int, char>::value) {
    return 2;
  }
  if (!SizedTrue<long>::value) {
    return 3;
  }
  if (accept_always(0) != 1) {
    return 4;
  }
  if (accept_true_alias(0) != 2) {
    return 5;
  }
  if (accept_sized(0) != 3) {
    return 6;
  }
  return 0;
}
