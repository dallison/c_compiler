// RUN: -std=c++17
// EXPECT: No such symbol "X"

template <typename T, typename T1>
constexpr int Find() {
  return 0;
}

struct B {
  template <typename U>
  int Off() const {
    return Find<U, X>();
  }
};

int main() {
  return 0;
}
