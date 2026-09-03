// RUN: -std=c++20
// EXPECT_EXIT: 0

template <class T>
struct Buffer {
  T* data;
  Buffer(unsigned long count) : data(new T[count]) {
    for (unsigned long i = 0; i < count; ++i) data[i] = static_cast<T>(i + 1);
  }
  ~Buffer() { delete[] data; }
};

template <class T>
void delete_null_pointers() {
  T* object = 0;
  T* array = 0;
  delete object;
  delete[] array;
}

int main() {
  Buffer<int> buffer(3);
  if (buffer.data[0] != 1 || buffer.data[2] != 3) return 1;
  delete_null_pointers<int>();
  return 0;
}
