// RUN: -std=c++20
// EXPECT_EXIT: 0

template <class T>
struct Holder {
  T* ptr;

  Holder() : ptr(nullptr) {}
  explicit Holder(T* p) : ptr(nullptr) {
    ptr = p;
  }

  Holder(const Holder& other) : ptr(nullptr) {
    ptr = other.ptr;
    ((Holder&)other).ptr = nullptr;
  }
};

int main() {
  int value = 17;
  Holder<int> first(&value);
  Holder<int> second = first;
  if (first.ptr != nullptr || second.ptr == nullptr) {
    return 1;
  }
  return *second.ptr == 17 ? 0 : 2;
}
