static int alias_read = 7;
static int alias_write;
static int sccp_global;

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

__attribute__((noinline)) int sccp_integer_extensions(void) {
  unsigned short narrow = 5;
  unsigned long long wide = 12;
  unsigned short truncated =
      (unsigned short)((unsigned long long)narrow - wide);
  signed char truthy = 4;
  return (truncated & 255) + !!truthy;
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
  if (sccp_executable_phi() != 16 ||
      sccp_equal_phi(0) != 11 || sccp_equal_phi(1) != 11 ||
      sccp_call_clobber() != 7 || sccp_integer_extensions() != 250) {
    result |= 1;
  }
  if (alias_distinct_globals(4) != 31 ||
      alias_distinct_globals(0) != 0) {
    result |= 2;
  }
  return result;
}
