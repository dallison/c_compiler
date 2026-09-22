// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <type_traits>

template <class Sig>
class Impl {};

template <class ReturnType, class... P>
class Impl<ReturnType(P...)> {
 public:
  template <class F>
  using CallIsValid = std::true_type;
};

template <class Sig, class F>
struct Probe {
  static constexpr bool value = Impl<Sig>::template CallIsValid<F>::value;
};

int main() {
  return Probe<void(), char>::value ? 0 : 1;
}
