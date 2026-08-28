#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
  // Enough allocations, and enough churn, to make the allocator split and
  // coalesce rather than just handing out fresh bytes.
  char* blocks[64];
  for (int i = 0; i < 64; i++) {
    blocks[i] = malloc((size_t)(i + 1) * 17);
    memset(blocks[i], 'a' + (i % 26), (size_t)(i + 1) * 17);
  }
  for (int i = 0; i < 64; i += 2) {
    free(blocks[i]);
  }
  for (int i = 0; i < 64; i += 2) {
    blocks[i] = calloc((size_t)(i + 1), 9);
  }

  int sum = 0;
  for (int i = 1; i < 64; i += 2) {
    sum += blocks[i][0];
  }
  printf("odd blocks still hold their fill: %d\n", sum);

  int zeroed = 1;
  for (int i = 0; i < 64; i += 2) {
    for (size_t j = 0; j < (size_t)(i + 1) * 9; j++) {
      if (blocks[i][j] != 0) {
        zeroed = 0;
      }
    }
  }
  printf("calloc zeroed: %d\n", zeroed);

  char* grown = malloc(8);
  strcpy(grown, "small");
  grown = realloc(grown, 512);
  printf("realloc kept: %s\n", grown);
  free(grown);

  for (int i = 0; i < 64; i++) {
    free(blocks[i]);
  }
  return 0;
}
