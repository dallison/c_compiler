// Sparse conditional constant propagation gave a local its own SSA versions
// whenever the renaming had consumed every direct reference to it, even though
// the address had escaped.  Binding a reference to the local leaves no direct
// reference behind, so the version the reference was taken from and the version
// the next assignment created came out as separate values, and the read after
// the callee had written through the reference folded to the last value stored
// here.

static void run(void (*fn)(void*), void* argument) {
  fn(argument);
}

static void add_two(void* p) {
  *static_cast<int*>(p) += 2;
}

static int through_function_pointer(void) {
  int value = 0;
  value = 5;
  run(add_two, &value);
  return value;
}

static void add_two_through_reference(int& target) {
  target += 2;
}

// The reference is bound before the assignment, which is what puts the escaping
// read and the store on different SSA versions.
static int through_reference_parameter(void) {
  int value = 0;
  int& alias = value;
  value = 5;
  add_two_through_reference(alias);
  return value;
}

static int through_pointer_variable(void) {
  int value = 0;
  int* address = &value;
  value = 5;
  run(add_two, address);
  return value;
}

int main(void) {
  if (through_function_pointer() != 7) {
    return 1;
  }
  if (through_reference_parameter() != 7) {
    return 2;
  }
  if (through_pointer_variable() != 7) {
    return 3;
  }
  return 0;
}
