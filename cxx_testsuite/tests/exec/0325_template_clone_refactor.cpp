// RUN: -std=c++23
// EXPECT_EXIT: 0

template <class T>
struct clone_box {
  T value;

  template <class F, class... Args>
  int apply(F&& function, Args&&... args) {
    if constexpr (sizeof...(Args) == 0) {
      return static_cast<F&&>(function)(value);
    } else {
      return static_cast<F&&>(function)(
          value, static_cast<Args&&>(args)...);
    }
  }
};

template <class T>
int size_branch(T value) {
  if constexpr (sizeof(T) == 1) {
    return value + 10;
  } else {
    T local = value;
    return local + 20;
  }
}

struct unary {
  int operator()(int value) const { return value + 1; }
};

struct ternary {
  int operator()(int first, int second, int third) const {
    return first + second + third;
  }
};

int main() {
  clone_box<int> first{4};
  clone_box<long> second{5};
  if (first.apply(unary{}) != 5) {
    return 1;
  }
  if (second.apply(ternary{}, 6, 7) != 18) {
    return 2;
  }
  if (size_branch<char>(1) != 11) {
    return 3;
  }
  if (size_branch<int>(2) != 22) {
    return 4;
  }
  // Reinstantiate an earlier specialization after different pack and
  // if-constexpr paths to catch accidental mutation of the template body.
  if (first.apply(ternary{}, 2, 3) != 9 ||
      second.apply(unary{}) != 6) {
    return 5;
  }
  return 0;
}
