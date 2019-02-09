//
//  p_code_interpreter.h
//  p_code_interpreter
//
//  Created by David Allison on 1/22/18.
//  Copyright © 2018 David Allison. All rights reserved.
//

#ifndef p_code_interpreter_h
#define p_code_interpreter_h

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
// Start of user escape codes.
#define P_CODE_ESC_USER_START  256

typedef struct Interpreter {
  Loader* loader;
  int64_t iregs[PCODE_NUM_INT_REGS];
  float fregs[PCODE_NUM_FLOAT_REGS];;
  double dregs[PCODE_NUM_DOUBLE_REGS];

  int32_t startup_code[4];
  char* stack;
  void (*escape)(struct Interpreter*, int32_t value);
  Symbol* current_symbol;
} Interpreter;

void InterpreterInit(Interpreter* interpreter, Loader* loader, uint64_t entry_address, int argc, char** argv);
void InterpreterDestruct(Interpreter* interpreter);
void InterpreterRun(Interpreter* interpreter);

#endif /* p_code_interpreter_h */
