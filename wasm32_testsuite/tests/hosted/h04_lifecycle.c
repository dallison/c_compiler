#include <stdio.h>
#include <stdlib.h>

// A constructor runs off the init array the linker builds out of the pieces
// each object contributes, so this is as much a test of the link as of libc.
static int constructed;

__attribute__((constructor)) static void Construct(void) {
  constructed = 1;
  printf("constructor ran\n");
}

static void Farewell(void) { printf("atexit ran\n"); }

int main(void) {
  printf("main sees constructed=%d\n", constructed);
  atexit(Farewell);
  printf("main returning\n");
  return 3;
}
