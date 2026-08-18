// RUN: -std=c++20
// EXPECT_EXIT: 0

template <typename T>
struct Box {
  T value;

  explicit Box(T input) : value(input) {}
};

template <template <typename> typename Container>
int construct_through_parameter() {
  Container object(42);
  return object.value;
}

template <typename... Ts>
struct Variadic {
  int count;

  explicit Variadic(Ts...) : count(sizeof...(Ts)) {}
};

template <template <typename...> typename Container>
int construct_variadic_through_parameter() {
  Container object(1, 2);
  return object.count;
}

template <typename T, typename U = char>
struct Defaulted {
  T value;
  U second;

  explicit Defaulted(T input) : value(input), second() {}
};

template <template <typename, typename = char> typename Container>
int construct_defaulted_through_parameter() {
  Container object(42);
  return object.value + sizeof(object.second);
}

template <typename... Ts>
struct ParameterDefault {
  int argument_count;

  explicit ParameterDefault(int) : argument_count(sizeof...(Ts)) {}
};

template <template <typename = char> typename Container>
int use_template_parameter_default() {
  Container object(42);
  return object.argument_count;
}

int main() {
  if (construct_through_parameter<Box>() != 42) {
    return 1;
  }
  if (construct_variadic_through_parameter<Variadic>() != 2) {
    return 2;
  }
  if (construct_defaulted_through_parameter<Defaulted>() != 43) {
    return 3;
  }
  return use_template_parameter_default<ParameterDefault>() == 1 ? 0 : 4;
}
