#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int Compare(const void* left, const void* right) {
  int a = *(const int*)left;
  int b = *(const int*)right;
  return a < b ? -1 : a > b ? 1 : 0;
}

int main(void) {
  char buffer[64];
  strcpy(buffer, "the quick brown fox");
  printf("len %zu\n", strlen(buffer));
  printf("chr %s\n", strchr(buffer, 'q'));
  printf("str %s\n", strstr(buffer, "brown"));
  printf("cmp %d %d\n", strcmp("abc", "abd") < 0, strncmp("abc", "abd", 2));

  strcat(buffer, " jumps");
  printf("cat %s\n", buffer);

  char sorted[] = "hello";
  memmove(sorted + 1, sorted, 4);
  printf("move %s\n", sorted);

  int values[] = {9, 3, 7, 1, 8, 2, 6, 4, 5, 0};
  qsort(values, 10, sizeof(int), Compare);
  for (int i = 0; i < 10; i++) {
    printf("%d", values[i]);
  }
  printf("\n");

  printf("atoi %d strtol %ld\n", atoi("-1234"), strtol("0x1f", NULL, 16));
  return 0;
}
