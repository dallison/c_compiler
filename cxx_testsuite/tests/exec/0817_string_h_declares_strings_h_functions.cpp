// As with glibc under g++'s implicit _GNU_SOURCE, <string.h> also declares the
// <strings.h> functions.
#include <string.h>

int main() {
  if (ffs(0) != 0) return 1;
  if (ffs(8) != 4) return 2;
  if (strcasecmp("Subspace", "SUBSPACE") != 0) return 3;
  if (strncasecmp("abcX", "ABCy", 3) != 0) return 4;
  return 0;
}
