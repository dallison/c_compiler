// Leaf functions with 6 integer arguments must not pin register variables to
// %r8/%r9: those hold the 5th and 6th SysV parameters.

static void add128(unsigned long ahi, unsigned long alo, unsigned long bhi,
                   unsigned long blo, unsigned long* rhi, unsigned long* rlo) {
  *rlo = alo + blo;
  *rhi = ahi + bhi + (*rlo < alo);
}

int main(void) {
  unsigned long rhi = 0;
  unsigned long rlo = 0;
  add128(1, 2, 3, 4, &rhi, &rlo);
  if (rlo != 6 || rhi != 4) {
    return 1;
  }
  rhi = 0;
  rlo = 0;
  add128(1, ~0UL, 5, 2, &rhi, &rlo);
  if (rlo != 1 || rhi != 7) {
    return 2;
  }
  return 0;
}
