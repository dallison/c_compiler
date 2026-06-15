// RUN: -std=c++20

int global_value = 3;
int* global_pointer = &global_value;

int main(void) {
  decltype(global_value) copy = 4;
  decltype((global_value)) ref = global_value;
  decltype(global_pointer) pointer = &copy;
  decltype(*global_pointer) pointer_ref = global_value;
  decltype(copy + global_value) sum = 1;
  decltype(static_cast<int&&>(global_value)) rvalue_ref =
      static_cast<int&&>(global_value);
  int& existing_ref = global_value;
  decltype(existing_ref) ref_copy = global_value;
  rvalue_ref = copy + *pointer + pointer_ref + ref_copy + sum;
  ref = rvalue_ref;
  return 0;
}
