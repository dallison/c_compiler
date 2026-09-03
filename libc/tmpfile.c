#include <stdio.h>

static unsigned long temporary_file_counter;

static char* MakeTemporaryName(char* buffer) {
  unsigned long value = temporary_file_counter++;
  snprintf(buffer, L_tmpnam, "/tmp/dv%08lx", value);
  return buffer;
}

char* tmpnam(char* output) {
  static char buffer[L_tmpnam];
  if (output == NULL) {
    output = buffer;
  }
  return MakeTemporaryName(output);
}

FILE* tmpfile(void) {
  char filename[L_tmpnam];
  FILE* stream;
  int attempts;
  for (attempts = 0; attempts < TMP_MAX; attempts++) {
    MakeTemporaryName(filename);
    stream = fopen(filename, "w+bx");
    if (stream != NULL) {
      if (remove(filename) == 0) {
        return stream;
      }
      fclose(stream);
      remove(filename);
    }
  }
  return NULL;
}
