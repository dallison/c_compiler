#include <cstdlib>
#include <cstdio>
#include <cstring>

static volatile int quick_state;

static void quick_check() {
  _Exit(quick_state == 1 ? 0 : 20);
}

static void quick_first() {
  if (quick_state != 2) _Exit(21);
  quick_state = 1;
}

static void quick_second() {
  if (quick_state != 0) _Exit(22);
  quick_state = 2;
}

int main() {
  char transformed[8];
  char overlap[] = "abcdef";
  char number[16];
  char filename[L_tmpnam];
  FILE* file;

  if (RAND_MAX != 32767) return 1;
  if (std::strcoll("abc", "abd") >= 0) return 2;
  if (std::strxfrm(transformed, "classic", sizeof(transformed)) != 7) return 3;
  if (std::strcmp(transformed, "classic") != 0) return 4;
  std::memmove(overlap + 1, overlap, 5);
  if (std::strcmp(overlap, "aabcde") != 0) return 5;
  if (std::memmove(overlap, overlap, 0) != overlap) return 6;
  if (strfromd(number, sizeof(number), "%.2f", 1.25) != 4) return 7;
  if (std::strcmp(number, "1.25") != 0) return 8;

  file = std::tmpfile();
  if (file == 0) return 9;
  if (std::fputc('Z', file) != 'Z') return 10;
  if (std::fclose(file) != 0) return 11;
  std::tmpnam(filename);
  file = std::fopen(filename, "w");
  if (file == 0 || std::fclose(file) != 0) return 15;
  if (std::fopen(filename, "wx") != 0) return 16;
  if (std::remove(filename) != 0) return 17;

  if (std::at_quick_exit(quick_check) != 0) return 12;
  if (std::at_quick_exit(quick_first) != 0) return 13;
  if (std::at_quick_exit(quick_second) != 0) return 14;
  std::quick_exit(19);
}
