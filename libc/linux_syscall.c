#include <stdarg.h>

long __davecc_linux_syscall6(long number, long a0, long a1, long a2, long a3,
                             long a4, long a5);
long __davecc_linux_syscall_result(long result);

long syscall(int number, ...) {
  va_list arguments;
  va_start(arguments, number);
  long a0 = va_arg(arguments, long);
  long a1 = va_arg(arguments, long);
  long a2 = va_arg(arguments, long);
  long a3 = va_arg(arguments, long);
  long a4 = va_arg(arguments, long);
  long a5 = va_arg(arguments, long);
  va_end(arguments);
  return __davecc_linux_syscall_result(
      __davecc_linux_syscall6(number, a0, a1, a2, a3, a4, a5));
}
