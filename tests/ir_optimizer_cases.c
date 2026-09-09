struct BoolAbiLayoutC {
  _Bool value;
  char next;
};
typedef char BoolAbiSizeCheck[sizeof(_Bool) == 1 ? 1 : -1];
typedef char BoolAbiLayoutCheck[sizeof(struct BoolAbiLayoutC) == 2 ? 1 : -1];

volatile int volatile_value = 3;
int loop_update;
static int induction_values[6] = {2, 3, 5, 7, 11, 13};
static int induction_output[6];
static int alias_read = 7;
static int alias_write;
static int sccp_global;
static int checkpoint_value;

__attribute__((noinline)) int update_loop_value(void) {
  return ++loop_update;
}

__attribute__((noinline)) int signed_arithmetic(int value) {
  if (value / 8 != -2 || value % 8 != -1) {
    return 1;
  }
  return 0;
}

__attribute__((noinline)) int unsigned_arithmetic(unsigned value) {
  return value / 8 == 2 && value % 8 == 1 ? 0 : 2;
}

__attribute__((noinline)) int phi_loop(int limit) {
  int sum = 0;
  int alternate = 1;
  for (int i = 0; i < limit; ++i) {
    if (i & 1) {
      sum += alternate;
    } else {
      alternate += 2;
    }
  }
  return sum + alternate;
}

__attribute__((noinline)) int short_circuit_and_copy(int left, int right) {
  int copied = left;
  int copied_again = copied;
  return copied_again && (right + 0);
}

__attribute__((noinline)) int loop_with_call(int count) {
  int result = 0;
  while (count-- != 0) {
    result += update_loop_value();
  }
  return result;
}

__attribute__((noinline)) int dead_expression(int value) {
  (void)((value + 17) * 9);
  return value;
}

__attribute__((noinline)) int licm_invariant(int value, int count) {
  int result = 0;
  for (int i = 0; i < count; ++i) {
    result += value * 9 + 7;
  }
  return result;
}

static int memopt_global;

__attribute__((noinline)) int nested_licm(int value) {
  int result = 0;
  for (int i = 0; i < 2; ++i) {
    for (int j = 0; j < 3; ++j) {
      result += value * 7 + 1;
    }
  }
  return result;
}

__attribute__((noinline)) int store_forward_global(void) {
  memopt_global = 11;
  return memopt_global;
}

__attribute__((noinline)) int load_cse_global(void) {
  return memopt_global + memopt_global;
}

__attribute__((noinline)) int dead_store_global(int value) {
  memopt_global = 1;
  memopt_global = value;
  return memopt_global;
}

static int unroll_values[4] = {1, 2, 3, 4};

typedef struct {
  int x;
  int y;
} SroaPoint;

__attribute__((noinline)) int sroa_point(int a, int b) {
  SroaPoint p;
  p.x = a;
  p.y = b;
  return p.x + p.y;
}

__attribute__((noinline)) int sroa_pair(int a, int b) {
  int v[2];
  v[0] = a;
  v[1] = b;
  return v[0] + v[1];
}

__attribute__((noinline)) int unroll_sum4(int *a) {
  int s = 0;
  for (int i = 0; i < 4; i++) {
    s += a[i];
  }
  return s;
}

__attribute__((noinline)) int combine_nested_add(int value) {
  return (value + 3) + 4;
}

__attribute__((noinline)) int combine_and_zero(int value) {
  return (value & 0) + (value | 0);
}

__attribute__((noinline)) unsigned udiv_const10(unsigned value) {
  return value / 10u;
}

__attribute__((noinline)) unsigned udiv_const7(unsigned value) {
  return value / 7u;
}

__attribute__((noinline)) int sdiv_const10(int value) {
  return value / 10;
}

__attribute__((noinline)) int induction_reload(int count) {
  int result = 0;
  for (int i = 0; i < count; ++i) {
    result += i;
  }
  return result;
}

__attribute__((noinline)) int induction_array_sum(int count) {
  int result = 0;
  for (int i = 0; i < count; ++i) {
    result += induction_values[i];
  }
  return result;
}

__attribute__((noinline)) int induction_array_store(int count) {
  for (int i = 0; i < count; ++i) {
    induction_output[i] = i * 3 + 1;
  }
  return induction_output[0] + induction_output[count - 1];
}

__attribute__((noinline)) int induction_reverse_sum(int count) {
  int result = 0;
  for (int i = count - 1; i >= 0; --i) {
    result = result * 10 + induction_values[i];
  }
  return result;
}

__attribute__((noinline)) int induction_nonunit_sum(int count) {
  int result = 0;
  for (int i = 0; i < count; i += 2) {
    result += induction_values[i];
  }
  return result;
}

__attribute__((noinline)) int sccp_executable_phi(void) {
  int selector = 6;
  int value;
  if ((selector & 3) == 2) {
    value = 11;
  } else {
    value = selector * 99;
  }
  return value + 5;
}

__attribute__((noinline)) int sccp_equal_phi(int selector) {
  int value;
  if (selector) {
    value = 9;
  } else {
    value = 9;
  }
  return value + 2;
}

__attribute__((noinline)) void sccp_mutate_global(void) {
  sccp_global = 7;
}

__attribute__((noinline)) int sccp_call_clobber(void) {
  sccp_global = 3;
  sccp_mutate_global();
  return sccp_global;
}

__attribute__((noinline)) int checkpoint_order(int* pointer) {
  checkpoint_value = 31;
  __builtin_observable_checkpoint();
  return checkpoint_value + *pointer;
}

__attribute__((noinline)) int alias_distinct_globals(int count) {
  int result = 0;
  alias_write = 0;
  for (int i = 0; i < count; ++i) {
    alias_write = i;
    result += alias_read;
  }
  return result + alias_write;
}

__attribute__((noinline)) int alias_may_alias(int count, int* left,
                                              int* right) {
  int result = 0;
  *right = 10;
  for (int i = 0; i < count; ++i) {
    *left = i;
    result += *right;
  }
  return result;
}

int main(void) {
  int result = 0;
  int checkpoint_input = 31;
  if (signed_arithmetic(-17) != 0) {
    result |= 1;
  }
  if (unsigned_arithmetic(17u) != 0) {
    result |= 2;
  }
  if (phi_loop(6) != 22) {
    result |= 4;
  }
  if (short_circuit_and_copy(1, 7) != 1 ||
      short_circuit_and_copy(0, 7) != 0) {
    result |= 8;
  }
  if (loop_with_call(3) != 6) {
    result |= 16;
  }
  if (volatile_value != 3 || dead_expression(5) != 5) {
    result |= 32;
  }
  if (licm_invariant(3, 4) != 136 || licm_invariant(3, 0) != 0 ||
      nested_licm(3) != 132) {
    result |= 64;
  }
  if (induction_reload(5) != 10) {
    result |= 128;
  }
  if (induction_array_sum(6) != 41 || induction_array_sum(0) != 0 ||
      induction_nonunit_sum(6) != 18) {
    result |= 64;
  }
  if (induction_array_store(4) != 11) {
    result |= 32;
  }
  if (induction_reverse_sum(4) != 7532 || induction_reverse_sum(0) != 0) {
    result |= 16;
  }
  memopt_global = 9;
  if (store_forward_global() != 11 || load_cse_global() != 22 ||
      dead_store_global(13) != 13 || memopt_global != 13 ||
      combine_nested_add(5) != 12 || combine_and_zero(7) != 7 ||
      udiv_const10(100u) != 10u || udiv_const10(9u) != 0u ||
      udiv_const7(0xffffffffu) != (0xffffffffu / 7u) ||
      sdiv_const10(-17) != -1 || sdiv_const10(99) != 9 ||
      unroll_sum4(unroll_values) != 10 || sroa_point(3, 4) != 7 ||
      sroa_pair(5, 6) != 11) {
    result |= 8;
  } else if (checkpoint_order(&checkpoint_input) != 62) {
    result |= 8;
  }
  return result;
}
