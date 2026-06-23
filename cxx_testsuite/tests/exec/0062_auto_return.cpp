// RUN: -std=c++20

int global_value = 7;

auto scalar_auto_return(void) {
  return 13;
}

auto* pointer_auto_return(void) {
  return &global_value;
}

auto& reference_auto_return(void) {
  return global_value;
}

auto void_auto_return(void) {
  global_value = global_value + 1;
  return;
}

int main(void) {
  if (scalar_auto_return() != 13) {
    return 1;
  }

  int* ptr = pointer_auto_return();
  if (ptr != &global_value || *ptr != 7) {
    return 2;
  }

  int& ref = reference_auto_return();
  ref = 21;
  if (global_value != 21) {
    return 3;
  }

  void_auto_return();
  if (global_value != 22) {
    return 4;
  }

  return 0;
}
