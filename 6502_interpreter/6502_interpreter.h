//
//  6502_intepreter.h
//  6502_interpreter
//
//  Created by David Allison on 5/18/19.
//  Copyright © 2019 David Allison. All rights reserved.
//

#ifndef W65C02_interpreter_h
#define W65C02_interpreter_h

#include "loader.h"
#include "6502_machine.h"
#include "6502_debugger.h"

#define W65C02_STACK_SIZE 1024

// Use undefined 65c02 instructions for special purposes.
#define W65C02_BRK 0xef            // syscall handler
#define W65C02_BREAKPOINT 0xff     // Breakpoint.

// Mapped I/O region.
#define W65C02_IO_START 0xfe00
#define W65C02_IO_END 0xfeff

#define W65C02_MAX_OPEN_FILES 10

struct W65C02Interpreter;

typedef struct W65C02Interpreter {
  Loader* loader;
  uint8_t* memory;         // 64K of memory
  uint8_t* zero_page;
  uint8_t* stack;
  uint8_t a;            // Accumulator.
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
  Vector watchpoints;
  bool debug;
  bool stop_at_next_instruction;
  Breakpoint* current_bp;
  char last_command[256];
  int next_bp_num;
  int entry_address;
  bool trace;
  Vector devices;
  bool cycle_accurate;
  uint16_t enter_func;        // Address of __enter (treated specially)
  uint16_t enter_leaf_func;        // Address of __enter_leaf (treated specially)
  uint16_t guest_call_return_pc;
  bool init_arrays_done;
  bool running;
  int exit_code;
  String rom_filename;
  int open_files[W65C02_MAX_OPEN_FILES];
} W65C02Interpreter;

bool W65C02GuestAddressExecutable(Loader* loader, uint16_t addr);
void W65C02GuestCallVoidFunction(W65C02Interpreter* interpreter, uint16_t fn);
bool W65C02GuestRunInitArrays(Loader* loader, W65C02Interpreter* interpreter);
bool W65C02GuestRunFiniArrays(Loader* loader, W65C02Interpreter* interpreter);

void W65C02InterpreterInit(W65C02Interpreter* interpreter, bool debug,
                           bool cycle_accurate,
                           bool trace, const char* rom_filename);

int W65C02InterpreterRun(W65C02Interpreter* interpreter, Loader* loader,
                         uint64_t entry_address, int argc, char** argv,
                         int first_arg);
void W65C02InterpreterDisassemble(W65C02Interpreter* interpreter, Loader* loader);
void W65C02InterpreterExtract(W65C02Interpreter* interpreter, Loader* loader, FILE* fp);
void W65C02InterpreterDestruct(W65C02Interpreter* interpreter);

void W65C02DisassemblePc(W65C02Interpreter* interpreter);
void W65C02Reset(W65C02Interpreter* interpreter);

#endif /* W65C02_interpreter_h */
