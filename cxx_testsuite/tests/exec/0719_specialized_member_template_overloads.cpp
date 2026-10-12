// RUN: -std=c++17
// EXPECT_EXIT: 0

template <class T>
struct Span {};

struct Container {
  template <int Index>
  int add(int*);

  template <int Index>
  int add(Span<int* const>);
};

template <>
int Container::add<0>(int*) {
  return 1;
}

template <>
int Container::add<0>(Span<int* const>) {
  return 2;
}

int main() {
  Container container;
  Span<int* const> values;
  return container.add<0>(values) == 2 ? 0 : 1;
}
