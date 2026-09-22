// RUN: -std=c++17
// EXPECT_EXIT: 0

template <typename T>
struct Storage {
  struct ElementwiseSwapPolicy {};
  struct ElementwiseConstructPolicy {};
  void SwapN(ElementwiseSwapPolicy, Storage* other, int n);
  void SwapN(ElementwiseConstructPolicy, Storage* other, int n);
  int value;
};

template <typename T>
void Storage<T>::SwapN(ElementwiseSwapPolicy, Storage*, int n) {
  value = n;
}

template <typename T>
void Storage<T>::SwapN(ElementwiseConstructPolicy, Storage*, int n) {
  value = n + 1;
}

int main() {
  Storage<int> a, b;
  a.SwapN(Storage<int>::ElementwiseSwapPolicy{}, &b, 3);
  int first = a.value;
  a.SwapN(Storage<int>::ElementwiseConstructPolicy{}, &b, 3);
  return first + a.value - 7;
}
