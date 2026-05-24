//
//  x86_64_runtime.c
//  x86_64_interpreter
//

#include "x86_64_runtime.h"
#include "x86_64_interpreter.h"
#include "x86_64_native.h"
#include "x86_64_syscalls.h"
#include "loader_arch_x86_64.h"
#include <stdlib.h>
#include <string.h>

bool print_libraries_only = false;

static void InitSymbolResolverCode(uint8_t* code) {
  // movabs rax, imm64
  code[0] = 0x48;
  code[1] = 0xB8;
  uint64_t resolve = X86_64_SYSCALL_RESOLVE;
  memcpy(code + 2, &resolve, 8);
  // syscall
  code[10] = 0x0F;
  code[11] = 0x05;
  memset(code + 12, 0, 4);
}

X86_64ExecutionMode X86_64DefaultExecutionMode(void) {
#if defined(__x86_64__)
  return kX86_64ModeNative;
#else
  return kX86_64ModeInterpret;
#endif
}

bool X86_64RuntimeInit(X86_64Runtime* runtime, const char* filename,
                       X86_64ExecutionMode mode, bool trace_registers,
                       bool trace_instructions) {
  memset(runtime, 0, sizeof(*runtime));
  runtime->mode = mode;
  runtime->trace_registers = trace_registers;
  runtime->trace_instructions = trace_instructions;

  String path = {0};
  StringInit(&path, filename);

  int32_t loader_flags = trace_instructions ? LOADER_MAP_SYMTAB : 0;
  char* bind_now = getenv("LD_BIND_NOW");
  if (mode == kX86_64ModeInterpret &&
      (bind_now == NULL || bind_now[0] == '\0')) {
    loader_flags |= LOADER_LAZY_RESOLVE;
  }
  char* ld_trace = getenv("LD_TRACE_LOADED_OBJECTS");
  if (ld_trace != NULL && ld_trace[0] != '\0') {
    print_libraries_only = true;
  }

  LoaderArchitecture* arch = &runtime->arch;
  X86_64LoaderArchitectureInit(arch);
  InitSymbolResolverCode(runtime->symbol_resolver_code);
  if (!LoaderInitFromFile(&runtime->loader, &path, loader_flags, arch,
                          runtime->symbol_resolver_code, ".")) {
    StringDestruct(&path);
    return false;
  }
  StringDestruct(&path);
  return true;
}

static int RunInterpreter(X86_64Runtime* runtime, int program_argc,
                          char** program_argv) {
  X86_64Interpreter interpreter;
  X86_64InterpreterInit(&interpreter, &runtime->loader,
                        runtime->loader.main_address, program_argc,
                        program_argv, runtime->trace_registers,
                        runtime->trace_instructions);
  int result = X86_64InterpreterRun(&interpreter);
  X86_64InterpreterDestruct(&interpreter);
  return result;
}

int X86_64RuntimeRun(X86_64Runtime* runtime, int argc, char** argv,
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

  if (runtime->mode == kX86_64ModeNative &&
      X86_64NativeNeedsInterpreter(&runtime->loader)) {
    return RunInterpreter(runtime, program_argc, program_argv);
  }

  if (runtime->mode == kX86_64ModeNative) {
    return X86_64NativeRun(&runtime->loader, runtime->loader.main_address,
                           program_argc, program_argv);
  }

  return RunInterpreter(runtime, program_argc, program_argv);
}

void X86_64RuntimeDestruct(X86_64Runtime* runtime) {
  LoaderDestruct(&runtime->loader);
}
