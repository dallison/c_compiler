//
//  6502_intepreter.h
//  6502_interpreter
//
//  Created by David Allison on 5/18/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#ifndef _6502_interpreter_h
#define _6502_interpreter_h

#include "loader.h"
#include "6502_machine.h"
#include "6502_debugger.h"

#define _6502_STACK_SIZE 1024

// Escape codes for interpreter.
#define _6502_ESC_UNDEF_INST 0
#define _6502_ESC_DIV_ZERO 1
#define _6502_ESC_WRITE 2       // Output an array.
#define _6502_ESC_READ 3        // Input an array.
#define _6502_ESC_HALT 4           // Halt interpreter.
#define _6502_ESC_DEBUG 5         // Debug escape.
#define _6502_ESC_RESOLVE 6      // Resolve symbol.
// Start of user escape codes.
#define _6502_ESC_USER_START  256

// Use undefined 65c02 instructions for special purposes.
#define _6502_BRK 0xef            // BRK handler
#define _6502_BREAKPOINT 0xff     // Breakpoint.

// Mapped I/O region.
#define _6502_IO_START 0xfe00
#define _6502_IO_END 0xfeff

struct _6502Interpreter;

typedef struct _6502Interpreter {
  Loader* loader;
  uint8_t* memory;         // 64K of memory
  uint8_t* zero_page;
  uint8_t* stack;
  int8_t a;             // Accumulator.
  uint8_t x;            // X index.
  uint8_t y;            // Y index.
  uint8_t s;            // 6502 stack pointer.
  uint16_t pc;          // Program Counter.
  union {
    struct {
      unsigned int c:1;               // Carry flag.
      unsigned int z:1;               // Zero flag.
      unsigned int i:1;               // Interrupt flag.
      unsigned int d:1;               // Decimal flag.
      unsigned int b:1;               // Break flag.
      unsigned int x:1;               // Not used.
      unsigned int v:1;               // Overflow flag.
      unsigned int s:1;               // Sign flag.
    } bits;
    int8_t value;
  } flags;
  
  SymbolScope* current_symbol;
  Vector breakpoints;
  bool debug;
  bool stop_at_next_instruction;
  Breakpoint* current_bp;
  char last_command[256];
  int next_bp_num;
  int entry_address;
  bool trace;
  Vector devices;
  bool cycle_accurate;
} _6502Interpreter;

void _6502InterpreterInit(_6502Interpreter* interpreter, bool debug, bool cycle_accurate);

void _6502InterpreterRun(_6502Interpreter* interpreter, Loader* loader, uint64_t entry_address, int argc, char** argv);
void _6502InterpreterDisassemble(_6502Interpreter* interpreter, Loader* loader);
void _6502InterpreterExtract(_6502Interpreter* interpreter, Loader* loader, FILE* fp);
void _6502InterpreterDestruct(_6502Interpreter* interpreter);

void _6502DisassemblePc(_6502Interpreter* interpreter);
void _6502Reset(_6502Interpreter* interpreter);

#endif /* _6502_interpreter_h */
