// RUN: -std=c++20

#include <stdarg.h>

static_assert(__builtin_expect(7, 1) == 7);
static_assert(sizeof(decltype(__builtin_expect((char)1, 0))) == sizeof(long));

static int copy_first_argument(int count, ...) {
  va_list original;
  va_list copy;
  va_start(original, count);
  va_copy(copy, original);
  int first = va_arg(original, int);
  int copied = va_arg(copy, int);
  va_end(copy);
  va_end(original);
  return first == copied ? first : -1;
}

int main(void) {
  int value = 3;
  int hint_side_effect = 0;
  long expected = __builtin_expect(value++, hint_side_effect++);
  if (expected != 3 || value != 4 || hint_side_effect != 1) {
    return 1;
  }

  int data[4] = {1, 2, 3, 4};
  int* next = data;
  __builtin_prefetch(next++);
  __builtin_prefetch(data + 1, 0);
  __builtin_prefetch(data + 2, 1, 2);
  if (next != data + 1) {
    return 2;
  }

  if (copy_first_argument(2, 17, 23) != 17) {
    return 3;
  }

  if (false) {
    __builtin_trap();
    __builtin_unreachable();
  }
  return 0;
}
