//
//  aarch64_runtime.h
//  aarch64_interpreter
//

#ifndef aarch64_runtime_h
#define aarch64_runtime_h

#include <stdbool.h>

#include "aarch64_process.h"
#include "loader.h"
#include "loader_arch.h"

struct AARCH64Interpreter;

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
  AARCH64ProcessRuntime process;
} AARCH64Runtime;

bool AARCH64RuntimeInit(AARCH64Runtime* runtime, const char* filename,
                        AARCH64ExecutionMode mode, bool trace_registers,
                        bool trace_instructions);
int AARCH64RuntimeRun(AARCH64Runtime* runtime, int argc, char** argv,
                      int arg_offset);
void AARCH64RuntimeDestruct(AARCH64Runtime* runtime);

AARCH64ExecutionMode AARCH64DefaultExecutionMode(void);

bool AARCH64GuestAddressExecutable(Loader* loader, uint64_t addr);
uint64_t AARCH64LookupGuestFunction(Loader* loader, const char* name);
void AARCH64GuestCallVoidFunction(struct AARCH64Interpreter* cpu, uint64_t fn);
void AARCH64GuestRunProgramFini(Loader* loader, struct AARCH64Interpreter* cpu);
bool AARCH64GuestRunProgramShutdown(Loader* loader,
                                    struct AARCH64Interpreter* cpu);
bool AARCH64GuestRunInitArrays(Loader* loader, struct AARCH64Interpreter* cpu);
bool AARCH64GuestRunFiniArrays(Loader* loader, struct AARCH64Interpreter* cpu);
int AARCH64NativeCallVoidFunction(Loader* loader, uint64_t fn);

#endif /* aarch64_runtime_h */
