// RUN: -std=c++17
// EXPECT_EXIT: 0

struct TrueType {
  static constexpr bool value = true;
};

template <class T>
struct Probe {
  static constexpr bool value = decltype(T())::value;
};

int main() {
  return Probe<TrueType>::value ? 0 : 1;
}
