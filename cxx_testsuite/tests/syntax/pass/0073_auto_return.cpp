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

auto trailing_scalar_return(int value) -> int {
  return value + 1;
}

auto trailing_pointer_return(void) -> int* {
  return &global_auto_return_value;
}

auto trailing_reference_return(void) -> int& {
  return global_auto_return_value;
}

auto trailing_declared_return(int value) -> int;

auto trailing_declared_return(int value) -> int {
  return value + 2;
}

struct TrailingReturnMember {
  int value;
  auto get(void) -> int {
    return value;
  }
};

void use_lambda_trailing_return_types(void) {
  int value = 9;
  auto scalar = [](int input) -> int {
    return input + 1;
  };
  auto pointer = [&value]() mutable noexcept -> int* {
    return &value;
  };

  int scalar_value = scalar(4);
  int* ptr = pointer();

  (void)scalar_value;
  (void)ptr;
}

void use_auto_return_types(void) {
  int scalar = scalar_auto_return();
  int* ptr = pointer_auto_return();
  int& ref = reference_auto_return();
  void_auto_return();
  int trailing_scalar = trailing_scalar_return(5);
  int* trailing_ptr = trailing_pointer_return();
  int& trailing_ref = trailing_reference_return();
  int trailing_declared = trailing_declared_return(6);
  TrailingReturnMember member = {7};
  int trailing_member = member.get();

  (void)scalar;
  (void)ptr;
  (void)ref;
  (void)trailing_scalar;
  (void)trailing_ptr;
  (void)trailing_ref;
  (void)trailing_declared;
  (void)trailing_member;
}
