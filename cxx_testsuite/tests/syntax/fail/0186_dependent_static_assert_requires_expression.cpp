// RUN: -std=c++20
// EXPECT: concept expected nested type
template <typename T>
concept HasType = requires { typename T::type; };

template <typename T>
int require_nested_type(void) {
  static_assert(HasType<T>, "concept expected nested type");
  return 1;
}

int main(void) {
  return require_nested_type<int>();
}
