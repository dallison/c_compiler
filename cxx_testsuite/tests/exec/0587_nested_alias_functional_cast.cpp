// RUN: -std=c++17
// EXPECT_EXIT: 0

struct MemcpyPolicy {
  int tag;
  MemcpyPolicy() : tag(7) {}
};

template <typename A>
struct Storage {
  using Policy = MemcpyPolicy;
  int Swap(Storage* other);
  int SwapInlinedElements(MemcpyPolicy p, Storage* other);
};

template <typename A>
int Storage<A>::Swap(Storage* other) {
  return SwapInlinedElements(Policy{}, other);
}

template <typename A>
int Storage<A>::SwapInlinedElements(MemcpyPolicy p, Storage*) {
  return p.tag;
}

int main() {
  Storage<int> a, b;
  return a.Swap(&b) - 7;
}
