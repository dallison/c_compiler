// RUN: -std=c++20
struct WithType {
  typedef int type;
};

template <typename T>
concept HasType = requires { typename T::type; };

template <typename T>
int require_nested_type(void) {
  static_assert(requires { typename T::type; }, "expected nested type");
  static_assert(HasType<T>, "concept expected nested type");
  return 1;
}

template <typename T>
int reject_missing_member(T value) {
  static_assert(!requires(T local) { local.missing; });
  return sizeof(value) - sizeof(value);
}

int main(void) {
  return require_nested_type<WithType>() + reject_missing_member(0) - 1;
}
