#include <stdio.h>
#include <string.h>

int main(void) {
  char value = 0;
  if (fread(&value, 0, 1, stdin) != 0) return 1;
  if (fread(&value, 1, 0, stdin) != 0) return 2;
  if (fwrite(&value, 0, 1, stdout) != 0) return 3;
  if (fwrite(&value, 1, 0, stdout) != 0) return 4;
  if (memcmp(NULL, NULL, 0) != 0) return 5;
  puts("ok");
  return 0;
}
