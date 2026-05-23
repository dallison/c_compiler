//
//  aarch64_runtime.c
//  aarch64_interpreter
//

#include "aarch64_runtime.h"
#include "aarch64_interpreter.h"
#include "aarch64_native.h"
#include "aarch64_syscalls.h"
#include "loader_arch_aarch64.h"
#include <stdlib.h>
#include <string.h>

bool print_libraries_only = false;

static void InitSymbolResolverCode(uint32_t* code) {
  // mov x16, #AARCH64_SYSCALL_RESOLVE
  code[0] = 0xD2800000u | ((uint32_t)AARCH64_SYSCALL_RESOLVE << 5) |
            (uint32_t)AARCH64_SYSCALL_REG;
  // svc #0
  code[1] = 0xD4000001u;
}

AARCH64ExecutionMode AARCH64DefaultExecutionMode(void) {
#if defined(__aarch64__)
  return kAARCH64ModeNative;
#else
  return kAARCH64ModeInterpret;
#endif
}

bool AARCH64RuntimeInit(AARCH64Runtime* runtime, const char* filename,
                        AARCH64ExecutionMode mode, bool trace_registers,
                        bool trace_instructions) {
  memset(runtime, 0, sizeof(*runtime));
  runtime->mode = mode;
  runtime->trace_registers = trace_registers;
  runtime->trace_instructions = trace_instructions;

  String path = {0};
  StringInit(&path, filename);

  int32_t loader_flags = trace_instructions ? LOADER_MAP_SYMTAB : 0;
  char* bind_now = getenv("LD_BIND_NOW");
  if (mode == kAARCH64ModeInterpret &&
      (bind_now == NULL || bind_now[0] == '\0')) {
    loader_flags |= LOADER_LAZY_RESOLVE;
  }
  char* ld_trace = getenv("LD_TRACE_LOADED_OBJECTS");
  if (ld_trace != NULL && ld_trace[0] != '\0') {
    print_libraries_only = true;
  }

  LoaderArchitecture* arch = &runtime->arch;
  AARCH64LoaderArchitectureInit(arch);
  InitSymbolResolverCode(runtime->symbol_resolver_code);
  if (!LoaderInitFromFile(&runtime->loader, &path, loader_flags, arch,
                          runtime->symbol_resolver_code, ".")) {
    StringDestruct(&path);
    return false;
  }
  StringDestruct(&path);
  return true;
}

static int RunInterpreter(AARCH64Runtime* runtime, int program_argc,
                          char** program_argv) {
  AARCH64Interpreter interpreter;
  AARCH64InterpreterInit(&interpreter, &runtime->loader,
                         runtime->loader.main_address, program_argc,
                         program_argv, runtime->trace_registers,
                         runtime->trace_instructions);
  int result = AARCH64InterpreterRun(&interpreter);
  AARCH64InterpreterDestruct(&interpreter);
  return result;
}

int AARCH64RuntimeRun(AARCH64Runtime* runtime, int argc, char** argv,
                      int arg_offset) {
  if (print_libraries_only) {
    return 0;
  }
  int program_argc = argc - arg_offset;
  char** program_argv = argv + arg_offset;
  if (program_argc <= 0) {
    static char* empty_argv[] = {NULL};
    program_argc = 0;
    program_argv = empty_argv;
  }

  if (runtime->mode == kAARCH64ModeNative &&
      AARCH64NativeNeedsInterpreter(&runtime->loader)) {
    return RunInterpreter(runtime, program_argc, program_argv);
  }

  if (runtime->mode == kAARCH64ModeNative) {
    return AARCH64NativeRun(&runtime->loader, runtime->loader.main_address,
                            program_argc, program_argv);
  }

  return RunInterpreter(runtime, program_argc, program_argv);
}

void AARCH64RuntimeDestruct(AARCH64Runtime* runtime) {
  LoaderDestruct(&runtime->loader);
}
