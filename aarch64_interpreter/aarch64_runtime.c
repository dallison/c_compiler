//
//  aarch64_runtime.c
//  aarch64_interpreter
//

#include "aarch64_runtime.h"
#include "aarch64_interpreter.h"
#include "aarch64_native.h"
#include "aarch64_syscalls.h"
#include "elf.h"
#include "loader_lifecycle.h"
#include "loader_arch_aarch64.h"
#include <stdio.h>
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

bool AARCH64GuestAddressExecutable(Loader* loader, uint64_t addr) {
  for (size_t i = 0; i < loader->regions.length; i++) {
    Region* region = loader->regions.value.p[i];
    if (region->segment == NULL) {
      continue;
    }
    if (region->segment->type != PT(load) ||
        (region->segment->flags & PF(x)) == 0) {
      continue;
    }
    uint64_t start = (uint64_t)(uintptr_t)region->address;
    uint64_t end = start + (uint64_t)region->length;
    if (addr >= start && addr < end) {
      return true;
    }
  }
  return false;
}

static bool GuestExecutableOrZero(Loader* loader, uint64_t addr) {
  return addr == 0 || AARCH64GuestAddressExecutable(loader, addr);
}

uint64_t AARCH64LookupGuestFunction(Loader* loader, const char* name) {
  uint64_t addr = LoaderLookupSymbol(loader, name);
  if (addr == 0 || !AARCH64GuestAddressExecutable(loader, addr)) {
    return 0;
  }
  return addr;
}

void AARCH64GuestCallVoidFunction(AARCH64Interpreter* cpu, uint64_t fn) {
  if (cpu == NULL || fn == 0) {
    return;
  }
  AARCH64InterpreterCall(cpu, fn);
}

void AARCH64GuestRunProgramFini(Loader* loader, AARCH64Interpreter* cpu) {
  AARCH64GuestCallVoidFunction(cpu,
                               AARCH64LookupGuestFunction(loader,
                                                          "__davecc_run_fini"));
}

bool AARCH64GuestRunProgramShutdown(Loader* loader, AARCH64Interpreter* cpu) {
  if (!LoaderLifecycleExecutableFiniAlreadyDone(loader->lifecycle)) {
    uint64_t guest_fini = AARCH64LookupGuestFunction(loader, "__davecc_run_fini");
    if (guest_fini != 0) {
      if (cpu != NULL) {
        AARCH64GuestCallVoidFunction(cpu, guest_fini);
      } else {
        AARCH64NativeCallVoidFunction(loader, guest_fini);
      }
      LoaderLifecycleMarkExecutableFiniComplete(loader, loader->lifecycle);
    }
  }
  return AARCH64GuestRunFiniArrays(loader, cpu);
}

static bool AARCH64LifecycleCallback(void* context, LoadedDynamicLibrary* image,
                                     uint64_t function,
                                     LoaderLifecyclePhase phase) {
  (void)image;
  (void)phase;
  typedef struct {
    Loader* loader;
    AARCH64Interpreter* cpu;
  } AARCH64LifecycleContext;
  AARCH64LifecycleContext* ctx = context;
  if (!AARCH64GuestAddressExecutable(ctx->loader, function)) {
    LoaderError("Function array entry 0x%llx is not executable\n",
                (unsigned long long)function);
    return false;
  }
  if (ctx->cpu != NULL) {
    AARCH64GuestCallVoidFunction(ctx->cpu, function);
  } else {
    AARCH64NativeCallVoidFunction(ctx->loader, function);
  }
  return true;
}

static bool RunGuestLifecyclePhase(Loader* loader, AARCH64Interpreter* cpu,
                                   LoaderLifecyclePhase phase) {
  typedef struct {
    Loader* loader;
    AARCH64Interpreter* cpu;
  } AARCH64LifecycleContext;
  AARCH64LifecycleContext ctx = {loader, cpu};
  return LoaderLifecycleRunPhase(loader, loader->lifecycle, phase,
                                 AARCH64LifecycleCallback, &ctx);
}

bool AARCH64GuestRunInitArrays(Loader* loader, AARCH64Interpreter* cpu) {
  return RunGuestLifecyclePhase(loader, cpu, kLoaderLifecyclePreinit) &&
         RunGuestLifecyclePhase(loader, cpu, kLoaderLifecycleInit);
}

bool AARCH64GuestRunFiniArrays(Loader* loader, AARCH64Interpreter* cpu) {
  return RunGuestLifecyclePhase(loader, cpu, kLoaderLifecycleFini);
}

int AARCH64NativeCallVoidFunction(Loader* loader, uint64_t fn) {
  if (fn == 0 || !GuestExecutableOrZero(loader, fn)) {
    return 0;
  }
#if defined(__aarch64__)
  typedef void (*GuestVoidFn)(void);
  GuestVoidFn guest_fn = (GuestVoidFn)(uintptr_t)fn;
  guest_fn();
#endif
  return 0;
}

static int RunInterpreter(AARCH64Runtime* runtime, int program_argc,
                          char** program_argv) {
  AARCH64Interpreter interpreter;
  AARCH64InterpreterInit(&interpreter, &runtime->loader,
                         runtime->loader.main_address, program_argc,
                         program_argv, runtime->trace_registers,
                         runtime->trace_instructions);
  if (!AARCH64GuestRunInitArrays(&runtime->loader, &interpreter)) {
    AARCH64InterpreterDestruct(&interpreter);
    return 1;
  }
  AARCH64InterpreterPrepareMain(
      &interpreter, runtime->loader.main_address, program_argc, program_argv,
      runtime->loader.is_static);
  int result = AARCH64InterpreterRun(&interpreter);
  if (!AARCH64GuestRunProgramShutdown(&runtime->loader, &interpreter)) {
    result = 1;
  }
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
