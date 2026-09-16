#include <stdio.h>
#include <stdlib.h>

int main(void) {
  int* value = malloc(sizeof(*value));
  if (value == NULL) {
    return 2;
  }
  *value = 42;
  printf("esp32:%d\n", *value);
  free(value);
  return 0;
}
