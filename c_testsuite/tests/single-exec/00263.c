#include <stdio.h>
#include <string.h>

// An octal escape takes at most three digits; following digits are ordinary
// characters (protobuf serializes descriptors as strings like "(\0132").
static const char kPieces[] = "(\0132" "\1234" "\0007" "\08";

int main(void) {
  static const char expected[] = {'(', 013, '2', 0123, '4', 0, '7', 0, '8', 0};
  if (sizeof(kPieces) != sizeof(expected)) return 1;
  if (memcmp(kPieces, expected, sizeof(expected)) != 0) return 2;
  if (sizeof("\1010") != 3 || "\1010"[0] != 'A') return 3;
  printf("ok\n");
  return 0;
}
