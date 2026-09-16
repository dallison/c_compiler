#include <stdio.h>

int main(void) {
  const char* path = "/tmp/davecc-wasm32-file-test.txt";
  FILE* file = fopen(path, "w");
  if (file == NULL) return 1;
  if (fputs("hi\n", file) < 0) return 2;
  if (fclose(file) != 0) return 3;

  file = fopen(path, "r");
  if (file == NULL) return 4;
  char buf[8];
  if (fgets(buf, sizeof(buf), file) == NULL) return 5;
  if (fclose(file) != 0) return 6;
  if (remove(path) != 0) return 7;
  printf("%s", buf);
  return 0;
}
