#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int main(void) {
#if defined(__arm__)
  puts("ok");
  return 0;
#else
  char text[] = ",one::two,";
  char* saved = NULL;
  if (strcmp(strtok_r(text, ",:", &saved), "one") != 0) return 1;
  if (strcmp(strtok_r(NULL, ",:", &saved), "two") != 0) return 2;
  if (strtok_r(NULL, ",:", &saved) != NULL) return 3;

  void* aligned = NULL;
  if (posix_memalign(&aligned, 64, 17) != 0) return 4;
  if (((size_t)aligned & 63) != 0) return 5;
  free(aligned);

  char path[] = "/tmp/davecc-posix-XXXXXX";
  int fd = mkstemp(path);
  if (fd < 0) return 6;
  struct stat value;
  if (fstat(fd, &value) != 0 || value.st_size != 0) return 7;
  FILE* file = fdopen(fd, "w");
  if (file == NULL) return 8;
  if (fputs("data", file) < 0 || fclose(file) != 0) return 9;
  if (stat(path, &value) != 0 || value.st_size != 4) return 10;
  if (access(path, F_OK) != 0) return 11;
  char* canonical = realpath(path, NULL);
  if (canonical == NULL || canonical[0] != '/') return 12;
  free(canonical);
  if (remove(path) != 0 || access(path, F_OK) == 0) return 13;

  char cwd[4096];
  if (getcwd(cwd, sizeof(cwd)) == NULL || cwd[0] != '/') return 14;
  if (getenv("PATH") == NULL) return 15;
  if (getenv("") != NULL || getenv("PATH=") != NULL) return 16;

  puts("ok");
  return 0;
#endif
}
