//
//  p_code_interpreter.h
//  p_code_interpreter
//
//  Created by David Allison on 1/22/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#ifndef p_code_interpreter_h
#define p_code_interpreter_h

#include <stdbool.h>
#include "loader.h"
#include "p_code_machine.h"

#define P_CODE_STACK_SIZE 8*1024*1024

// Escape codes for interpreter.
#define P_CODE_ESC_UNDEF_INST 0
#define P_CODE_ESC_DIV_ZERO 1
#define P_CODE_ESC_WRITE 2       // Output an array.
#define P_CODE_ESC_READ 3        // Input an array.
#define P_CODE_ESC_HALT 4           // Halt interpreter.
#define P_CODE_ESC_DEBUG 5         // Debug escape.
#define P_CODE_ESC_RESOLVE 6      // Resolve symbol.
#define P_CODE_ESC_SYSCALL 7      // Generic guest syscall bridge.
#define P_CODE_ESC_ABORT 11
#define P_CODE_ESC_EXIT 12
#define P_CODE_ESC_EXIT_CLEAN 22
// Start of user escape codes.
#define P_CODE_ESC_USER_START  256
#define P_CODE_ESC_PROGRAM_RETURN P_CODE_ESC_USER_START

typedef struct PCodeInterpreter {
  Loader* loader;
  int64_t iregs[PCODE_NUM_INT_REGS];
  float fregs[PCODE_NUM_FLOAT_REGS];;
  double dregs[PCODE_NUM_DOUBLE_REGS];

  int32_t startup_code[4];
  int32_t symbol_resolver_code[1];
  
  char* stack;
  void (*escape)(struct PCodeInterpreter*, int32_t value);
  SymbolScope* current_symbol;
  bool running;
  int exit_code;
} PCodeInterpreter;

void PCodeInterpreterInit(PCodeInterpreter* interpreter);
void PCodeInterpreterSetDisassemble(bool enabled);

int PCodeInterpreterRun(PCodeInterpreter* interpreter, Loader* loader, uint64_t entry_address, int argc, char** argv);
void PCodeInterpreterCall(PCodeInterpreter* interpreter, uint64_t fn);
void PCodeInterpreterDestruct(PCodeInterpreter* interpreter);

bool PCodeGuestAddressExecutable(Loader* loader, uint64_t addr);
uint64_t PCodeLookupGuestFunction(Loader* loader, const char* name);
void PCodeGuestCallVoidFunction(PCodeInterpreter* interpreter, uint64_t fn);
bool PCodeGuestRunInitArrays(Loader* loader, PCodeInterpreter* interpreter);
bool PCodeGuestRunFiniArrays(Loader* loader, PCodeInterpreter* interpreter);
void PCodeGuestRunProgramFini(Loader* loader, PCodeInterpreter* interpreter);
bool PCodeGuestRunProgramShutdown(Loader* loader,
                                  PCodeInterpreter* interpreter);

#endif /* p_code_interpreter_h */
