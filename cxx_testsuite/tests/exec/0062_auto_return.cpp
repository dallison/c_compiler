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

auto trailing_scalar_return(int value) -> int {
  return value + 3;
}

auto trailing_pointer_return(void) -> int* {
  return &global_value;
}

auto trailing_reference_return(void) -> int& {
  return global_value;
}

auto trailing_declared_return(int value) -> int;

auto trailing_declared_return(int value) -> int {
  return value + 4;
}

struct TrailingReturnMember {
  int value;
  auto get(void) -> int {
    return value;
  }
};

int lambda_trailing_return_total(void) {
  int value = 12;
  auto scalar = [](int input) -> int {
    return input + 1;
  };
  auto pointer = [&value]() mutable noexcept -> int* {
    return &value;
  };

  int total = scalar(4);
  *pointer() = *pointer() + 3;
  return total + value;
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

  if (trailing_scalar_return(5) != 8) {
    return 5;
  }

  int* trailing_ptr = trailing_pointer_return();
  if (trailing_ptr != &global_value || *trailing_ptr != 22) {
    return 6;
  }

  int& trailing_ref = trailing_reference_return();
  trailing_ref = 30;
  if (global_value != 30) {
    return 7;
  }

  if (trailing_declared_return(6) != 10) {
    return 8;
  }

  TrailingReturnMember member = {11};
  if (member.get() != 11) {
    return 9;
  }

  if (lambda_trailing_return_total() != 20) {
    return 10;
  }

  return 0;
}
