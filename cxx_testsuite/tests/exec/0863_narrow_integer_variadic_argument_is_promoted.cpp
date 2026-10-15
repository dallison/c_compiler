// A variadic argument narrower than int is promoted to int, extended by its
// own signedness.  Narrowing a sign-extended enum to `uint8_t` keeps the
// register's upper bits until that promotion, so printing abseil's control
// bytes as `(uint8_t)ctrl` showed 0xffffff80 instead of 0x80.
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum class Ctrl : int8_t { kEmpty = -128, kSentinel = -1 };
enum Plain : int8_t { kPlainNegative = -2 };

__attribute__((noinline)) Ctrl Get(int i) {
  return i != 0 ? Ctrl::kEmpty : Ctrl::kSentinel;
}

static int Nth(int n, ...) {
  va_list ap;
  va_start(ap, n);
  int value = 0;
  for (int i = 0; i <= n; i++) value = va_arg(ap, int);
  va_end(ap);
  return value;
}

int main() {
  Ctrl empty = Get(1);
  Ctrl sentinel = Get(0);
  if (Nth(0, (uint8_t)empty) != 0x80) return 1;
  if (Nth(0, static_cast<uint8_t>(sentinel)) != 0xff) return 2;
  if (Nth(0, (uint16_t)empty) != 0xff80) return 3;
  if (Nth(1, 7, empty) != -128) return 4;
  if (Nth(0, (unsigned char)kPlainNegative) != 0xfe) return 5;
  if (Nth(0, (int8_t)(uint8_t)empty) != -128) return 6;
  if (Nth(0, (bool)Get(1)) != 1) return 7;
  char text[16];
  snprintf(text, sizeof text, "%02x", (uint8_t)empty);
  if (strcmp(text, "80") != 0) return 8;
  return 0;
}
