// RUN: -std=c++26
// EXPECT_EXIT: 0

struct Pair {
  int first;
  int second;
};

template <int value>
int template_reference() {
  constexpr int local = value;
  constexpr const int& reference = local;
  static_assert(reference == value);
  return reference;
}

int main() {
  constexpr int value = 42;
  constexpr const int& reference = value;
  constexpr const int* pointer = &value;
  static_assert(reference == 42);
  static_assert(*pointer == 42);

  constexpr int array[2] = {17, 25};
  constexpr auto [array_first, array_second] = array;
  static_assert(array_first == 17);
  static_assert(array_second == 25);

  constexpr auto [class_first, class_second] = Pair{20, 22};
  static_assert(class_first == 20);
  static_assert(class_second == 22);

  int runtime_value = 4;
  constexpr int& runtime_reference = runtime_value;
  runtime_reference = 5;

  constinit auto [initialized_first, initialized_second] = Pair{2, 3};

  auto lambda = [] {
    constexpr int local = 7;
    constexpr const int& reference = local;
    static_assert(reference == 7);
    return reference;
  };

  return reference + array_first + array_second + class_first + class_second ==
                     126 &&
                 runtime_value + initialized_first + initialized_second == 10
                 && template_reference<42>() == 42 && lambda() == 7
             ? 0
             : 1;
}
