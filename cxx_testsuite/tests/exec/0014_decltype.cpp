int global_value = 5;
int* global_pointer = &global_value;

int main(void) {
  decltype(global_value) copy = 7;
  decltype((global_value)) ref = global_value;
  decltype(global_pointer) pointer = &copy;
  decltype(*global_pointer) pointer_ref = global_value;
  decltype(copy + global_value) sum = 1;
  int& existing_ref = global_value;
  decltype(existing_ref) ref_copy = global_value;

  ref = copy + *pointer + pointer_ref + ref_copy + sum;
  if (global_value != 25) {
    return 1;
  }
  pointer_ref = 23;
  if (global_value != 23) {
    return 2;
  }
  ref_copy = 31;
  if (global_value != 31) {
    return 3;
  }
  return 0;
}
