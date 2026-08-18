// RUN: -std=c++26
// EXPECT_EXIT: 0

template <typename T>
concept Basic = sizeof(T) > 0;

template <typename T>
concept Refined = Basic<T> && true;

template <typename... Ts>
  requires (Basic<Ts> && ...)
int select(Ts...) {
  return 1;
}

template <typename... Ts>
  requires (Refined<Ts> && ...)
int select(Ts...) {
  return 2;
}

template <typename... Ts>
  requires ((sizeof(Ts) > 0) && ...)
int accepts_empty_pack() {
  return 3;
}

int main() {
  if (select(1, 2, 3) != 2) {
    return 1;
  }
  return accepts_empty_pack<>() == 3 ? 0 : 2;
}
