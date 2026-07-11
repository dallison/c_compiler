// RUN: -std=c++20
// EXPECT_EXIT: 0

template <class T>
struct Owner {
  struct Handle {
    T* ptr = nullptr;
  };

  T value;

  Handle make() {
    Handle result;
    result.ptr = &value;
    return result;
  }
};

int main() {
  Owner<int> owner;
  owner.value = 23;
  Owner<int>::Handle handle = owner.make();
  if (handle.ptr == nullptr) {
    return 1;
  }
  return *handle.ptr == 23 ? 0 : 2;
}
