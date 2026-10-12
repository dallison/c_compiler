// RUN: -std=c++17
// EXPECT_EXIT: 0

#include <type_traits>

class Target;

class Friend {
 public:
  template <typename T>
  struct Nested {
    using type = typename T::Secret;
  };

  template <typename T>
  static char Probe(const typename T::Secret*);

  template <typename T>
  static double Probe(...);

  template <typename T>
  struct HasSecret
      : std::integral_constant<
            bool, sizeof(Probe<T>(static_cast<const T*>(nullptr))) ==
                      sizeof(char)> {};
};

class Target {
  template <typename>
  friend struct Friend::Nested;
  template <typename>
  friend struct Friend::HasSecret;

  using Secret = void;
};

static_assert(std::is_same<Friend::Nested<Target>::type, void>::value);
static_assert(Friend::HasSecret<Target>::value);

int main() {
  return 0;
}
