// RUN: -std=c++17

int* take_int_pointer(int* value) {
  return value;
}

void* take_void_pointer(void* value) {
  return value;
}

int main(void) {
  int value = 42;
  int* pointer = &value;
  void* erased = pointer;
  void* erased_from_function = take_void_pointer(pointer);
  int* restored = static_cast<int*>(erased);
  int* c_style_restored = (int*)erased_from_function;
  int* argument = take_int_pointer(static_cast<int*>(erased));
  return *restored + *c_style_restored + *argument;
}
