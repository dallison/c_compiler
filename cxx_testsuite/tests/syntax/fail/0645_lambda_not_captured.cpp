// RUN: -std=c++20
// EXPECT: 'x' is not captured

template <class T>
struct Hold {
  template <class U>
  int run(U x) {
    auto lambda = [this](auto&&...) {
      if constexpr (sizeof(T) == 1) {
        return 1;
      } else {
        return (int)x;
      }
    };
    return lambda();
  }
};

int main() {
  Hold<int> hold;
  return hold.run(3);
}
