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

  if (initialized != &value || assigned != &value || argument != &value ||
      returned != &value) {
    return 1;
  }
  return *returned != 42;
}
