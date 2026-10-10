#include <stdio.h>
#include <string.h>

int main(void) {
  char buf[64];

  // A null %p is spelled out in place; the rest of the format still follows.
  snprintf(buf, sizeof(buf), "a=%p b=%p end", (void*)0, (void*)16);
  if (strcmp(buf, "a=(null) b=0x10 end") != 0) {
    return 1;
  }

  snprintf(buf, sizeof(buf), "[%10p]", (void*)0);
  if (strcmp(buf, "[    (null)]") != 0) {
    return 2;
  }
  return 0;
}
