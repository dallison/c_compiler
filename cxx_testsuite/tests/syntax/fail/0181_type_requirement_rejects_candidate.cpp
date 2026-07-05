// RUN: -std=c++20
// EXPECT: constraints not satisfied
// EXPECT: because concept HasNestedType was not satisfied [with T = int]
// EXPECT: because this type requirement was not satisfied
template <typename T>
concept HasNestedType = requires { typename T::type; };

template <typename T>
requires HasNestedType<T>
int use_nested_type(T value) {
  return sizeof(value);
}

int main(void) {
  return use_nested_type(0);
}
