#ifndef bpf_interpreter_h
#define bpf_interpreter_h

#include <stdbool.h>
#include <stdint.h>
#include "loader.h"

#define BPF_INTERP_MAX_FRAMES 32
#define BPF_INTERP_STACK 8192

typedef struct {
  uint64_t ret_pc;
  uint64_t r6;
  uint64_t r7;
  uint64_t r8;
  uint64_t r9;
  uint64_t fp;
  char* stack;
} BPFCallFrame;

typedef struct {
  Loader* loader;
  uint64_t r[11];
  uint64_t pc;
  char* stack;
  BPFCallFrame frames[BPF_INTERP_MAX_FRAMES];
  int depth;
  bool running;
  bool trace;
  int exit_code;
  int32_t symbol_resolver_code[1];
} BPFInterpreter;

void BPFInterpreterInit(BPFInterpreter* interp);
void BPFInterpreterDestruct(BPFInterpreter* interp);
int BPFInterpreterRun(BPFInterpreter* interp, Loader* loader, uint64_t entry);

#endif
