#include <stdio.h>

// Logical not of an operand that is a constant zero once lowered (an
// enumerator, a const object, a folded builtin) must still yield 1.
static const int kZero = 0;
static const long kZeroLong = 0;
enum { kOff, kOn };

static int NotEnumerator(int* counter) {
  if (!kOff) *counter += 1;
  return *counter;
}

static int NotConstObject(int* counter) {
  if (!kZero) *counter += 2;
  if (!kZeroLong) *counter += 4;
  return *counter;
}

static int NotBuiltin(int* value) { return !__builtin_constant_p(*value); }

static int NotEnumeratorValue(void) {
  int flag = !kOff;
  return flag;
}

int main(void) {
  int counter = 0;
  if (NotEnumerator(&counter) != 1) return 1;
  if (NotConstObject(&counter) != 7) return 2;
  if (NotBuiltin(&counter) != 1) return 3;
  if (NotEnumeratorValue() != 1) return 4;
  if (!kOn) return 5;
  printf("ok\n");
  return 0;
}
