/* __attribute__((packed)) and ((aligned(N))) layout.
   The printed values are chosen to be independent of the target's int/long
   width so the single .expected file matches every backend. */
#include <stdio.h>

struct __attribute__((packed)) Packed { char a; long b; char c; };
typedef struct { char a; long b; } __attribute__((packed)) TrailPacked;
struct __attribute__((aligned(16))) Aligned16 { char c; };

int main(void) {
  struct Packed p;
  /* In a packed struct the long sits immediately after the leading char. */
  printf("%d\n", (int)((char *)&p.b - (char *)&p));
  /* size = 1 (char) + sizeof(long) + 1 (char), so size - sizeof(long) == 2. */
  printf("%d\n", (int)(sizeof(struct Packed) - sizeof(long)));

  /* Same, via the trailing (typedef) attribute form. */
  TrailPacked t;
  printf("%d\n", (int)((char *)&t.b - (char *)&t));
  printf("%d\n", (int)(sizeof(TrailPacked) - sizeof(long)));

  /* aligned(16) rounds the struct size up to 16. */
  printf("%d\n", (int)sizeof(struct Aligned16));
  return 0;
}
