#include <stdio.h>
#include <string.h>

static const char* select_name(const char* name) {
  char scratch[4096];
  snprintf(scratch, sizeof(scratch), "%s", name);
  const char* selected = strcmp(name, "-") == 0 ? "stdin" : name;
  return selected + (scratch[0] == '\0');
}

int main(void) {
  const char* selected = select_name("ok");
  puts(selected);
  return strcmp(selected, "ok") != 0;
}
