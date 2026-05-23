//
//  aarch64_runtime.h
//  aarch64_interpreter
//

#ifndef aarch64_runtime_h
#define aarch64_runtime_h

#include <stdbool.h>

#include "loader.h"
#include "loader_arch.h"

typedef enum {
  kAARCH64ModeInterpret,
  kAARCH64ModeNative,
} AARCH64ExecutionMode;

typedef struct AARCH64Runtime {
  Loader loader;
  LoaderArchitecture arch;
  AARCH64ExecutionMode mode;
  bool trace_instructions;
  bool trace_registers;
  uint32_t symbol_resolver_code[2];
} AARCH64Runtime;

bool AARCH64RuntimeInit(AARCH64Runtime* runtime, const char* filename,
                        AARCH64ExecutionMode mode, bool trace_registers,
                        bool trace_instructions);
int AARCH64RuntimeRun(AARCH64Runtime* runtime, int argc, char** argv,
                      int arg_offset);
void AARCH64RuntimeDestruct(AARCH64Runtime* runtime);

AARCH64ExecutionMode AARCH64DefaultExecutionMode(void);

#endif /* aarch64_runtime_h */
