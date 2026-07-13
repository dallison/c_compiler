// RUN: -std=c++20
// EXPECT_EXIT: 0

template <typename T>
concept Always = true;

template <typename T>
struct Box {
  int constrained() requires Always<T> { return 1; }
  int constrained(int value) { return value; }
};

int main() {
  Box<int> box{};
  return box.constrained(7) == 7 ? 0 : 1;
}
