// RUN: -std=c++20
// EXPECT: constraints not satisfied

template <typename T>
concept Never = false;

template <typename T>
struct Wrapper {
  void rejected() requires Never<T> {
  }
};

int main() {
  Wrapper<int> wrapper;
  wrapper.rejected();
}
