// RUN: -std=c++26

template <typename T>
T&& forward_reference(T&& value) {
  return static_cast<T&&>(value);
}

int&& indirect_temporary() {
  return forward_reference(42);
}

static_assert(__davecc_is_convertible(int, const double&));
