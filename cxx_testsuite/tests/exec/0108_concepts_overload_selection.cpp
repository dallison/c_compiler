// RUN: -std=c++20
// EXPECT_EXIT: 0

template <typename T>
concept Large = sizeof(T) > 4;

template <typename T>
concept Addable = requires(T a, T b) {
  { a + b };
};

struct WithNestedType {
  typedef int type;
};

template <typename T>
concept HasNestedType = requires { typename T::type; };

template <typename T>
int constrained_pick(T value) {
  return 1 + sizeof(value) - sizeof(value);
}

template <typename T>
requires Large<T>
int constrained_pick(T value) {
  return 2 + sizeof(value) - sizeof(value);
}

template <typename T>
requires (Large<T>)
int subsumption_pick(T value) {
  return 3 + sizeof(value) - sizeof(value);
}

template <typename T>
requires (Large<T> && Addable<T>)
int subsumption_pick(T value) {
  return 4 + sizeof(value) - sizeof(value);
}

int abbreviated_pick(Large auto value) {
  return 5 + sizeof(value) - sizeof(value);
}

int trailing_requires_pick(auto value) requires Large<decltype(value)> {
  return 6 + sizeof(value) - sizeof(value);
}

template <typename T>
requires HasNestedType<T>
int nested_type_pick(T value) {
  return 7 + sizeof(value) - sizeof(value);
}

int nested_type_pick(int value) {
  return 8 + sizeof(value) - sizeof(value);
}

int main(void) {
  if (constrained_pick((char)0) != 1) {
    return 1;
  }
  if (constrained_pick((long long)0) != 2) {
    return 2;
  }
  if (subsumption_pick((long long)0) != 4) {
    return 3;
  }
  if (abbreviated_pick((long long)0) != 5) {
    return 4;
  }
  if (trailing_requires_pick((long long)0) != 6) {
    return 5;
  }
  auto lambda = [](Large auto value) {
    return 9 + sizeof(value) - sizeof(value);
  };
  if (lambda((long long)0) != 9) {
    return 6;
  }
  auto trailing_lambda = [](auto value) requires Large<decltype(value)> {
    return 10 + sizeof(value) - sizeof(value);
  };
  if (trailing_lambda((long long)0) != 10) {
    return 7;
  }
  WithNestedType with_type;
  if (nested_type_pick(with_type) != 7) {
    return 8;
  }
  if (nested_type_pick(0) != 8) {
    return 9;
  }
  return 0;
}
