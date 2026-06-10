/* #pragma pack(n) / pack() / pack(push[,n]) / pack(pop).

   Verifies that the pack pragma caps struct member alignment (and therefore
   member offsets and struct size), that pack() resets to the default, and that
   push/pop save and restore the value.  Also reads and writes through a
   packed (potentially misaligned) member to confirm codegen, not just the
   computed layout. */
#include <stdio.h>

struct Normal { char c; int i; };

#pragma pack(1)
struct Packed1 { char c; int i; };
#pragma pack()

#pragma pack(push, 2)
struct Packed2 { char c; int i; short s; };
#pragma pack(pop)

struct AfterPop { char c; int i; };

#define OFF(s, m) ((int)((char*)&(s).m - (char*)&(s)))

int main(void) {
  struct Normal n;
  struct Packed1 p1;
  struct Packed2 p2;
  struct AfterPop a;

  printf("%d %d\n", (int)sizeof(struct Normal), OFF(n, i));
  printf("%d %d\n", (int)sizeof(struct Packed1), OFF(p1, i));
  printf("%d %d %d\n", (int)sizeof(struct Packed2), OFF(p2, i), OFF(p2, s));
  printf("%d %d\n", (int)sizeof(struct AfterPop), OFF(a, i));

  p1.c = 'A';
  p1.i = 0x12345678;
  printf("%c %x\n", p1.c, p1.i);
  return 0;
}
