// RUN: -std=c++20
// EXPECT_EXIT: 0

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
  if (has_nested_type<WithType>() != 1) {
    return 1;
  }
  if (has_nested_type<int>() != 0) {
    return 2;
  }
  if (has_missing_member(0) != 0) {
    return 3;
  }
  if (has_missing_member(value) != 1) {
    return 4;
  }
  return 0;
}
