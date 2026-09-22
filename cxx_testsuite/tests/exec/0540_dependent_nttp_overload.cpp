// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <type_traits>

template <typename T>
T ForwardImpl(std::true_type);

template <typename T>
T&& ForwardImpl(std::false_type);

template <class T>
struct ForwardedParameter {
  using type = decltype((ForwardImpl<T>)(
      std::integral_constant<bool, std::is_scalar<T>::value>()));
};

struct NotScalar {
  int x;
};

int main() {
  if (!std::is_same<ForwardedParameter<int>::type, int>::value) {
    return 1;
  }
  if (!std::is_same<ForwardedParameter<NotScalar>::type, NotScalar&&>::value) {
    return 2;
  }
  return 0;
}
