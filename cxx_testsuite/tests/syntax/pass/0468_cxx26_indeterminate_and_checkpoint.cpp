// RUN: -std=c++26

#include <utility>

#if __cpp_lib_observable_checkpoint != 202506L
#error "observable checkpoint library feature macro has the wrong value"
#endif

void first([[indeterminate]] int parameter);
void first(int parameter) {}
void trailing(int parameter [[indeterminate]]);
void trailing(int parameter) {}

void accepted() {
  [[indeterminate]] int scalar;
  int trailing_scalar [[indeterminate]];
  int values[3];
  values[1] = 42;
  unsigned char byte;
  unsigned char copied = byte;
  (void)scalar;
  (void)trailing_scalar;
  (void)values;
  (void)copied;
  std::observable_checkpoint();
  __builtin_observable_checkpoint();
}
