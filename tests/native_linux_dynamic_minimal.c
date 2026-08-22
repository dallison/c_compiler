#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
  void* allocation = malloc(97);
  if (allocation == NULL) return 1;
  memset(allocation, 0x5a, 97);
  free(allocation);

  errno = 0;
  int fd = open("/definitely/not/a/davecc/dynamic/file", O_RDONLY);
  if (fd != -1 || errno != ENOENT) return 2;
  return strcmp("native-loader", "native-loader") != 0;
}
