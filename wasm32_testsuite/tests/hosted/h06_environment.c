// argv and the environment both come from the host through WASI rather than
// off a stack the program was started on, so this checks that _start put them
// where main and getenv expect to find them.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv) {
  printf("argc %d\n", argc);
  printf("argv0 %s\n", argc > 0 && argv[0] != NULL ? "present" : "missing");
  const char* home = getenv("DAVECC_WASM_TEST_VAR");
  printf("var %s\n", home == NULL ? "unset" : home);
  return 0;
}
