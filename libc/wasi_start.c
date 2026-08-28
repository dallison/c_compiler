//
//  wasi_start.c
//  c_compiler
//
//  Where a wasm32 program begins.
//
//  A WASI command is a module exporting '_start' with no arguments and no
//  result; the host calls it and treats its return as a clean exit.  There
//  is no stack to set up and no arguments arrive on one, so unlike the crt0
//  of a real machine this only has to collect the command line, run the
//  initializers and call main.
//

// Every target's libc archive is built from all of libc, so this has to be
// nothing at all anywhere else: its '_start' would otherwise stand against
// the crt0 of a machine that has one.
#if defined(__wasm32__)

#include <stdlib.h>

extern int main(int argc, char** argv, char** envp);
extern void __davecc_program_init(void);
extern void __davecc_environ_init(char** environment);

extern int __wasi_args_sizes_get(unsigned* argc, unsigned* buffer_size);
extern int __wasi_args_get(char** argv, char* buffer);
extern int __wasi_environ_sizes_get(unsigned* count, unsigned* buffer_size);
extern int __wasi_environ_get(char** environ, char* buffer);

// WASI hands over a vector of pointers and the bytes they point into, both
// of which the caller has to find room for.  Asking for the sizes first
// means one allocation each and no guessing.
static char** CollectStrings(int (*sizes)(unsigned*, unsigned*),
                             int (*get)(char**, char*), unsigned* count) {
  unsigned buffer_size = 0;
  *count = 0;
  if (sizes(count, &buffer_size) != 0) {
    *count = 0;
    return NULL;
  }
  // One past the end for the null terminator the vector needs.
  char** strings = malloc((*count + 1) * sizeof(char*));
  char* buffer = buffer_size > 0 ? malloc(buffer_size) : NULL;
  if (strings == NULL || (buffer_size > 0 && buffer == NULL)) {
    free(strings);
    free(buffer);
    *count = 0;
    return NULL;
  }
  if (get(strings, buffer) != 0) {
    free(strings);
    free(buffer);
    *count = 0;
    return NULL;
  }
  strings[*count] = NULL;
  return strings;
}

void _start(void) {
  unsigned argc = 0;
  char** argv = CollectStrings(__wasi_args_sizes_get, __wasi_args_get, &argc);
  unsigned environment_count = 0;
  char** envp = CollectStrings(__wasi_environ_sizes_get, __wasi_environ_get,
                               &environment_count);

  // A host that gave us nothing still has to leave main with a vector it can
  // walk, so an empty one stands in for a failed collection.
  static char* no_strings[1] = {NULL};
  if (argv == NULL) {
    argv = no_strings;
    argc = 0;
  }
  if (envp == NULL) {
    envp = no_strings;
  }

  __davecc_environ_init(envp);
  __davecc_program_init();
  exit(main((int)argc, argv, envp));
}

#endif /* __wasm32__ */
