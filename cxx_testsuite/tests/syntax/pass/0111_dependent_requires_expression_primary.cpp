// RUN: -std=c++20
struct WithType {
  typedef int type;
  int missing;
};

template <typename T>
int has_nested_type(void) {
  return requires { typename T::type; } ? 1 : 0;
}

template <typename T>
int has_missing_member(T value) {
  return requires(T local) { local.missing; } ? 1 : 0;
}

int main(void) {
  WithType value;
  return has_nested_type<WithType>() + has_missing_member(0) +
         has_missing_member(value) - 2;
}
