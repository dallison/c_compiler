#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

void TestStrings() {
  char buf1[256];
  char buf2[256];
  strcpy(buf1, "hello world");
  strcpy(buf2, "hello ");
  strcat(buf2, "world");
  printf("buf1: %s, buf2: %s\n", buf1, buf2);
  int v = strcmp(buf1, buf2);
  printf("%-20d %20sfoo\n", v, "foobar");
}

int main(int argc, char** argv) {
  TestStrings();
  for (int i = 0; i < 30; i++) {
    printf("test: %08x (%d)\n", i, i);
  }
}
