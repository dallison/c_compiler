int dirty_int(void);
unsigned dirty_uint(void);

int main(void) {
  int r = dirty_int();
  if (r != -5) return 1;
  if (r > 0) return 2;
  long l = r;
  if (l != -5) return 3;
  unsigned u = dirty_uint();
  if (u != 7) return 4;
  unsigned long ul = u;
  if (ul != 7) return 5;
  if (u / 7 != 1) return 6;
  if (dirty_int() / 5 != -1) return 7;
  if ((dirty_uint() >> 1) != 3) return 8;
  return 0;
}
