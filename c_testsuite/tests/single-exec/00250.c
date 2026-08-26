#include <stdio.h>

// An empty comment closes with the two characters that follow "/*".  The
// preprocessor's comment scan skipped the first of them, so the close had
// already gone by when it started looking for one and it consumed the rest of
// the input instead.

#define SUM 1 /**/ + /**/ 2

int main(void) {
  int x = 1 /**/ + 2;
  /**/
  int y = SUM;
  /**/ int z = 4 /**/;
  int w = 5 /* not empty */ + 6;

  if (x != 3) return 1;
  if (y != 3) return 2;
  if (z != 4) return 3;
  if (w != 11) return 4;

  puts("ok");
  return 0;
}
