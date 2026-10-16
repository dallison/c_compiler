// RUN: -std=c++20
template<typename T>
struct Box {
  T* ptr;
  Box(T& value) : ptr(&value) {}
  T& get() { return *ptr; }
};

int main() {
  int x = 7;
  Box<int> box(x);
  if (box.get() != 7) {
    return 1;
  }
  const int y = 3;
  Box<const int> const_box(y);
  if (const_box.get() != 3) {
    return 2;
  }
  return 0;
}
