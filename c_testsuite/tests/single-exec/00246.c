#include <stdio.h>

int main(void) {
  const char* path = "/tmp/davecc-fseek-buffer-test";
  FILE* file = fopen(path, "w+");
  if (file == NULL) return 1;
  if (fputs("abcdef", file) < 0) return 2;
  if (fseek(file, 0, SEEK_SET) != 0) return 3;
  if (fgetc(file) != 'a') return 4;
  if (ftell(file) != 1) return 5;
  if (fseek(file, 2, SEEK_CUR) != 0) return 6;
  if (fgetc(file) != 'd') return 7;
  if (ftell(file) != 4) return 8;
  if (fclose(file) != 0) return 9;
  if (remove(path) != 0) return 10;
  puts("ok");
  return 0;
}
