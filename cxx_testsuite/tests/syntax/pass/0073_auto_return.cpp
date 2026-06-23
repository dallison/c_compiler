// RUN: -std=c++20

int global_auto_return_value = 5;

auto scalar_auto_return(void) {
  return 11;
}

auto* pointer_auto_return(void) {
  return &global_auto_return_value;
}

auto& reference_auto_return(void) {
  return global_auto_return_value;
}

auto void_auto_return(void) {
  return;
}

void use_auto_return_types(void) {
  int scalar = scalar_auto_return();
  int* ptr = pointer_auto_return();
  int& ref = reference_auto_return();
  void_auto_return();

  (void)scalar;
  (void)ptr;
  (void)ref;
}
