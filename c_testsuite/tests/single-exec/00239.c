int puts(const char*);

int global_value = 9;

int persistent_local_static(void) {
  static int value = 17;
  return ++value;
}

int *static_address(void) {
  static int *pointer = &global_value;
  return pointer;
}

int main(void) {
  if (persistent_local_static() != 18) {
    return 1;
  }
  if (persistent_local_static() != 19) {
    return 2;
  }
  if (static_address() != &global_value || *static_address() != 9) {
    return 3;
  }
  puts("local static ok");
  return 0;
}
