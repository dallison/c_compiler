// RUN: -std=c++17
// EXPECT: Illegal conversion; cannot convert from

int* take_int_pointer(int* value) {
  return value;
}

int* return_int_pointer_from_void(void* value) {
  return value;
}

int main(void) {
  int value = 42;
  void* erased = &value;
  int* initialized = erased;
  int* assigned = 0;
  assigned = erased;
  int* argument = take_int_pointer(erased);
  int* returned = return_int_pointer_from_void(erased);
  return *initialized + *assigned + *argument + *returned;
}
