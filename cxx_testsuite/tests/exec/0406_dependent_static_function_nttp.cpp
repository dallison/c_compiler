// RUN: -std=c++20
// EXPECT_EXIT: 0

template <typename T>
struct Provider {
  static int add(T value) {
    return static_cast<int>(value) + 7;
  }
};

int noexcept_add(int value) noexcept {
  return value + 3;
}

template <auto Function>
int invoke(int value) {
  return Function(value);
}

template <typename T, int (*Function)(T)>
struct TypedInvoker {
  static int call(T value) {
    return Function(value);
  }
};

template <typename T>
int call_dependent_function(int value) {
  return invoke<&Provider<T>::add>(value);
}

template <typename T>
int call_dependent_typed_function(T value) {
  return TypedInvoker<T, &Provider<T>::add>::call(value);
}

int main() {
  if (call_dependent_function<int>(5) != 12) {
    return 1;
  }
  if (call_dependent_typed_function<int>(9) != 16) {
    return 2;
  }
  if (TypedInvoker<int, &noexcept_add>::call(4) != 7) {
    return 3;
  }
  return 0;
}
