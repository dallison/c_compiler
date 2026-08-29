// Binding a reference to a local is the only thing that takes its address, and
// the front end does not record it as address-of.  At -O2 the variable was left
// eligible for a register: the reference then held the variable's value as if it
// were an address, and reads of the variable were folded to the value it was
// given before the reference wrote through it.

int through_lvalue_reference(void) {
  int a = 3;
  int& r = a;
  r = 17;
  return a;
}

int through_rvalue_reference(void) {
  int a = 3;
  int&& r = static_cast<int&&>(a);
  r = 17;
  return a;
}

int through_const_reference(void) {
  int a = 3;
  const int& r = a;
  a = 17;
  return r;
}

int through_pointer(void) {
  int a = 3;
  int* p = &a;
  *p = 17;
  return a;
}

int add_through_reference(int start) {
  int total = start;
  int& acc = total;
  for (int i = 1; i <= 4; i++) {
    acc += i;
  }
  return total;
}

int reference_to_reference(void) {
  int a = 3;
  int& first = a;
  int& second = first;
  second = 17;
  return a;
}

struct Pair {
  int left;
  int right;
};

int reference_to_member(void) {
  Pair pair = {1, 2};
  int& right = pair.right;
  right = 17;
  return pair.left + pair.right;
}

int main(void) {
  if (through_lvalue_reference() != 17) {
    return 1;
  }
  if (through_rvalue_reference() != 17) {
    return 2;
  }
  if (through_const_reference() != 17) {
    return 3;
  }
  if (through_pointer() != 17) {
    return 4;
  }
  if (add_through_reference(0) != 10) {
    return 5;
  }
  if (reference_to_reference() != 17) {
    return 6;
  }
  if (reference_to_member() != 18) {
    return 7;
  }
  return 0;
}
