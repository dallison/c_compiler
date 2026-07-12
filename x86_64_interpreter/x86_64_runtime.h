//
//  x86_64_runtime.h
//  x86_64_interpreter
//

#ifndef x86_64_runtime_h
#define x86_64_runtime_h

#include <stdbool.h>
#include <stdint.h>

#include "loader.h"
#include "loader_arch.h"
#include "x86_64_process.h"

typedef enum {
  kX86_64ModeInterpret,
  kX86_64ModeNative,
} X86_64ExecutionMode;

typedef struct X86_64Runtime {
  Loader loader;
  LoaderArchitecture arch;
  X86_64ProcessRuntime process;
  X86_64ExecutionMode mode;
  bool trace_instructions;
  bool trace_registers;
  uint8_t symbol_resolver_code[16];
} X86_64Runtime;

bool X86_64RuntimeInit(X86_64Runtime* runtime, const char* filename,
                       X86_64ExecutionMode mode, bool trace_registers,
                       bool trace_instructions);
int X86_64RuntimeRun(X86_64Runtime* runtime, int argc, char** argv,
                     int arg_offset);
void X86_64RuntimeDestruct(X86_64Runtime* runtime);

X86_64ExecutionMode X86_64DefaultExecutionMode(void);

#endif /* x86_64_runtime_h */
