// Standalone atoi.
//
// Deliberately not implemented as (int)strtol(...): strtol parses in
// unsigned long with overflow tracking, which drags the wide multiply and
// divide runtime into small programs.  atoi has no defined overflow
// behaviour, so plain int arithmetic is sufficient.

int atoi(const char* s) {
  while (*s == ' ' || (*s >= '\t' && *s <= '\r')) {
    s++;
  }
  int negative = 0;
  if (*s == '+' || *s == '-') {
    negative = *s++ == '-';
  }
  // Accumulate negated so INT_MIN parses without overflow.
  int value = 0;
  while (*s >= '0' && *s <= '9') {
    value = value * 10 - (*s++ - '0');
  }
  return negative ? value : -value;
}
