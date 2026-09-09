// RUN: -std=c++20

template <class T>
struct Wrapper {
  T value;
};

template <class T, class C = char>
struct Formatter {
  static constexpr int id = 1;
};

template <class T>
struct Formatter<Wrapper<T>, char> {
  static constexpr int id = 2;
};

void use_nested_partial_spec() {
  Formatter<Wrapper<int>, char> specialized;
  Formatter<int, char> primary;
  (void)specialized;
  (void)primary;
  static_assert(Formatter<Wrapper<int>, char>::id == 2, "");
  static_assert(Formatter<int, char>::id == 1, "");
}
