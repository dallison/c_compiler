// RUN: -std=c++20
// EXPECT_EXIT: 0

template <typename T>
concept Never = false;

template <typename T>
struct Holder {
  static constexpr int value = 1;
};

template <typename T>
  requires Never<T>
struct Holder<T*> {
  static constexpr int value = 2;
};

int main() {
  return Holder<int*>::value == 1 ? 0 : 1;
}
