//
//  risc_v_interpreter.h
//  risc_v_interpreter
//
//  Created by David Allison on 4/23/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#ifndef risc_v_interpreter_h
#define risc_v_interpreter_h

#include "loader.h"
#include "risc_v_machine.h"

#define RISC_V_STACK_SIZE 8*1024*1024

// Environment call codes (passed in t6(x31)).  These are the equivalent of
// system calls in an OS.
#define RISC_V_ECALL_HALT 1
#define RISC_V_ECALL_OPEN 2
#define RISC_V_ECALL_CLOSE 3
#define RISC_V_ECALL_WRITE 4
#define RISC_V_ECALL_READ 5
#define RISC_V_ECALL_RESOLVE 6

typedef struct Interpreter {
  Loader* loader;
  int64_t iregs[RV_NUM_INT_REGS];
  double fregs[RV_NUM_FLOAT_REGS];;
  int64_t old_iregs[RV_NUM_INT_REGS];
  double old_fregs[RV_NUM_FLOAT_REGS];;

  int32_t startup_code[3];
  int32_t symbol_resolver_code[2];
  char* stack;
  int64_t pc;
  SymbolScope* current_symbol;
  bool trace_regs;
  bool trace_instructions;
} Interpreter;

void InterpreterInit(Interpreter* interpreter, bool trace_regs, bool trace_instructions);
void InterpreterRun(Interpreter* interpreter, Loader* loader, uint64_t entry_address, int argc, char** argv);
void InterpreterDestruct(Interpreter* interpreter);

#endif /* risc_v_interpreter_h */
