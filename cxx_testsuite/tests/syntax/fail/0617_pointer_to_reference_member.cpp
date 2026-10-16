// RUN: -std=c++20
// EXPECT: 'ptr' declared as a pointer to a reference

// `T*` with `T = int&` forms a pointer to a reference.
template <typename T>
struct Box {
  T* ptr;
};

int main() {
  Box<int&> box;
  (void)box;
  return 0;
}
