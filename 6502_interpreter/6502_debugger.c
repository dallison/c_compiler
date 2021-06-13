//
//  6502_debugger.c
//  6502_interpreter
//
//  Created by David Allison on 6/4/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#include "6502_debugger.h"
#include "6502_interpreter.h"

enum DebugCommand {
  kDbgNone,
  kDbgEmpty,
  kDbgQuit,
  kDbgRegs,
  kDbgMemory,
  kDbgSetBreak,
  kDbgDelBreak,
  kDbgListBreak,
  kDbgContinue,
  kDbgRun,
  kDbgStep,
  kDbgStepOver,
  kDbgDasm,
  kDbgTrace,
  kDbgHelp,
};

static struct {
  const char* spelling;
  enum DebugCommand cmd;
} commands[] = {
  {"quit", kDbgQuit},
  {"q", kDbgQuit},
  {"regs", kDbgRegs},
  {"mem", kDbgMemory},
  {"dump", kDbgMemory},
  {"b", kDbgSetBreak},
  {"run", kDbgRun},
  {"c", kDbgContinue},
  {"s", kDbgStep},
  {"n", kDbgStepOver},
  {"dasm", kDbgDasm},
  {"del", kDbgDelBreak},
  {"bps", kDbgListBreak},
  {"trace", kDbgTrace},
  {"help", kDbgHelp},
};

#define NUM_COMMANDS (sizeof(commands) / sizeof(commands[0]))

static void Help() {
  printf("%-20s Quit interpreter\n", "quit");
  printf("%-20s Show Registers\n", "regs");
  printf("%-20s tHex dump memory\n", "mem [addr] [len");
  printf("%-20s Hex dump memory\n", "dump [addr] [len");
  printf("%-20s Disassemble\n", "dasm [addr] [len]");
  printf("%-20s Set breakpoint at addr\n", "b addr");
  printf("%-20s Delete breakpoint number\n", "del bpnum");
  printf("%-20s List breakpoints\n", "bps");
  printf("%-20s tRun from beginning\n", "run");
  printf("%-20s Continue\n", "c");
  printf("%-20s Single step\n", "s");
  printf("%-20s Single step over\n", "n");
  printf("%-20s Switch tracing on or off\n", "trace on|off");
}

static enum DebugCommand ParseCommand(const char** cmd) {
  const char* p = *cmd;
  char buf[256];
  char* s = buf;
  while (!isblank(*p) && *p != '\n') {
    *s++ = *p++;
  }
  *s = '\0';
  if (buf[0] == '\0') {
    return kDbgEmpty;
  }
  for (int i = 0; i < NUM_COMMANDS; i++) {
    if (strcmp(commands[i].spelling, buf) == 0) {
      *cmd = p;
      return commands[i].cmd;
    }
  }
  return kDbgNone;
}

void Hexdump(_6502Interpreter* interpreter, const void* addr, int size) {
  char buf[16];
  const char* caddr = (const char*)addr;
  const char* endaddr = caddr + size;
  int len = size;
  while (len > 0) {
    int iaddr = (int)((char*)caddr - (char*)interpreter->memory);
    printf("%04X  ", iaddr);
    int filled_bytes = 0;
    for (int i = 0; i < 16; i++) {
      if (caddr < endaddr) {
         buf[i] = *caddr++;
        filled_bytes++;
        printf("%02X ", buf[i] & 0xff);
      } else {
        printf("   ");
      }
    }
    printf("  ");
    for (int i = 0; i < filled_bytes; i++) {
      if (buf[i] >= 0x20 && buf[i] < 0x7f) {
        printf("%c", buf[i]);
      } else {
        printf("%c", '.');
      }
    }
    len -= 16;
    printf("\n");
  }
}

static void DumpMemory(_6502Interpreter* interpreter, const char* tail) {
  int start = interpreter->pc;
  int len = 20;
  
  while (isspace(*tail)) {
    tail++;
  }
  char buf[256];
  char* p = buf;
  while (!isblank(*tail) && *tail != '\n' && *tail != '\0') {
    *p++ = *tail++;
  }
  *p = '\0';
  if (buf[0] != '\0') {
    start = (int)strtoll(buf, NULL, 0);
  } else {
    while (isspace(*tail)) {
      tail++;
    }
    p = buf;
    while (!isblank(*tail) && *tail != '\n' && *tail != '\0') {
      *p++ = *tail++;
    }
    *p = '\0';
    if (buf[0] != '\0') {
      len = (int)strtoll(buf, NULL, 0);
    }
  }
  Hexdump(interpreter, &interpreter->memory[start], len);
}

static void PrintFlag(int v, const char nm) {
  if (v) {
    putchar(nm);
  } else {
    putchar(tolower(nm));
  }
}

static void DumpRegs(_6502Interpreter* interpreter, const char* tail) {
  printf("A:  0x%02x\n", interpreter->a & 0xff);
  printf("X:  0x%02x\n", interpreter->x & 0xff);
  printf("Y:  0x%02x\n", interpreter->y & 0xff);
  printf("S:  0x%02x\n", interpreter->s & 0xff);
  printf("PC: 0x%02x\n", interpreter->pc  & 0xffff);
  printf("P:  ");
  PrintFlag(interpreter->flags.bits.c, 'C');
  PrintFlag(interpreter->flags.bits.z, 'Z');
  PrintFlag(interpreter->flags.bits.i, 'I');
  PrintFlag(interpreter->flags.bits.d, 'D');
  PrintFlag(interpreter->flags.bits.b, 'B');
  PrintFlag(interpreter->flags.bits.x, 'X');
  PrintFlag(interpreter->flags.bits.v, 'V');
  PrintFlag(interpreter->flags.bits.s, 'S');
  printf("\n");
}

Breakpoint* NewBreakpoint(int num, int addr, char old_value, bool temp) {
  Breakpoint* bp = malloc(sizeof(Breakpoint));
  bp->num = num;
  bp->addr = addr;
  bp->old_value = old_value;
  bp->enabled = false;
  bp->is_temp = temp;
  return bp;
}

void BreakpointDestruct(Breakpoint* bp) {
}

void BreakpointDelete(Breakpoint* bp) {
  free(bp);
}

void BreakpointInstall(_6502Interpreter* interpreter, Breakpoint* bp) {
  interpreter->memory[bp->addr] = _6502_BREAKPOINT;
  bp->enabled = true;
}

void BreakpointUninstall(_6502Interpreter* interpreter, Breakpoint* bp) {
  interpreter->memory[bp->addr] = bp->old_value;
  bp->enabled = false;
}

Breakpoint* FindBreakpoint(_6502Interpreter* interpreter, int addr) {
  for (size_t i = 0; i < interpreter->breakpoints.length; i++) {
    Breakpoint* bp = interpreter->breakpoints.value.p[i];
    if (bp->addr == addr && bp->enabled) {
      return bp;
    }
  }
  return NULL;
}

static int FindSymbolAddress(_6502Interpreter* interpreter, const char* name) {
  return (int)LoaderLookupSymbol(interpreter->loader, name);
}

static void CreateBreakpoint(_6502Interpreter* interpreter, int addr, bool temp) {
  Breakpoint* bp = FindBreakpoint(interpreter, addr);
  if (bp != NULL) {
    printf("Breakpoint %d already set at this address, ignoring\n", bp->num);
    return;
  }
  bp = NewBreakpoint(interpreter->next_bp_num++, addr, interpreter->memory[addr], temp);
  VectorAppend(&interpreter->breakpoints, bp);
  BreakpointInstall(interpreter, bp);
  if (!temp) {
    printf("Breakpoint %d set at address 0x%04x\n", bp->num, bp->addr);
  }
}

static void SetBreakpoint(_6502Interpreter* interpreter, const char* tail) {
  while (isspace(*tail)) {
    tail++;
  }
  if (*tail == '*') {
    // By address.
    tail++;
    uint64_t addr = strtoull(tail, NULL, 0);
    if (addr == 0) {
      printf("Invalid breakpoint address\n");
      return;
    }
    CreateBreakpoint(interpreter, (int)addr, false);
    return;
  }
  char symbol_name[256];
  char* s = symbol_name;
  while (*tail != '\0' && !isspace(*tail)) {
    *s++ = *tail++;
  }
  *s = 0;
  int addr = FindSymbolAddress(interpreter, symbol_name);
  if (addr == 0) {
    printf("Unknown symbol %s", symbol_name);
    return;
  }
  CreateBreakpoint(interpreter, (int)addr, false);;
}

void RemoveBreakpoint(_6502Interpreter* interpreter, Breakpoint* bp) {
  for (size_t i = 0; i < interpreter->breakpoints.length; i++) {
    Breakpoint* b = interpreter->breakpoints.value.p[i];
    if (b ==  bp) {
      VectorDeleteElement(&interpreter->breakpoints, i);
      return;
    }
  }
  printf("No such breakpoint %p\n", bp);
}

static void RemoveBreakpointByNumber(_6502Interpreter* interpreter, int num) {
  for (size_t i = 0; i < interpreter->breakpoints.length; i++) {
    Breakpoint* bp = interpreter->breakpoints.value.p[i];
    if (bp->num ==  num) {
      VectorDeleteElement(&interpreter->breakpoints, i);
      return;
    }
  }
  printf("No such breakpoint %d\n", num);
}


static void DeleteBreakpoint(_6502Interpreter* interpreter, const char* tail) {
  while (isspace(*tail)) {
    tail++;
  }
  if (*tail == '\0') {
    printf("Delete all breakpoints (y/n)?");
    fflush(stdout);
    char ch = fgetc(stdin);
    if (ch == 'y' || ch == 'Y') {
      for (size_t i = 0; i < interpreter->breakpoints.length; i++) {
        Breakpoint* bp = interpreter->breakpoints.value.p[i];
        if (bp->enabled) {
          BreakpointUninstall(interpreter, bp);
        }
        BreakpointDelete(bp);
      }
      VectorClear(&interpreter->breakpoints);
      return;
    }
  }
  int bpnum = (int)strtoll(tail, NULL, 0);
  RemoveBreakpointByNumber(interpreter, bpnum);
}

static void ListBreakpoints(_6502Interpreter* interpreter, const char* tail) {
  for (size_t i = 0; i < interpreter->breakpoints.length; i++) {
    Breakpoint* bp = interpreter->breakpoints.value.p[i];
    printf("%-2d 0x%x\n", bp->num, bp->addr);
  }
}

static void Step(_6502Interpreter* interpreter, const char* tail) {
  interpreter->stop_at_next_instruction = true;
}

static void StepOver(_6502Interpreter* interpreter, const char* tail) {
  if (interpreter->memory[interpreter->pc] == 0x20) {
    // JSR instruction.
    uint16_t next = interpreter->pc + 3;
    // Create temp breakpoint at next address (after JSR).
    CreateBreakpoint(interpreter, next, true);
    interpreter->stop_at_next_instruction = false;
    return;
  }
  interpreter->stop_at_next_instruction = true;
}

static void Continue(_6502Interpreter* interpreter, const char* tail) {
  interpreter->stop_at_next_instruction = false;
}

static bool Run(_6502Interpreter* interpreter, const char* tail) {
  if (interpreter->pc != 0xff00) {
    printf("Run from beginning (y/n)?");
    fflush(stdout);
    char ch = fgetc(stdin);
    if (ch == 'y' || ch == 'Y') {
      _6502Reset(interpreter);
      return true;
    }
    return false;
  }
  _6502Reset(interpreter);
  return true;
}

static void Disassemble(_6502Interpreter* interpreter, const char* tail) {
  int start = interpreter->pc;
  int len = 20;
  
  while (isspace(*tail)) {
    tail++;
  }
  char buf[256];
  char* p = buf;
  while (!isblank(*tail) && *tail != '\n' && *tail != '\0') {
    *p++ = *tail++;
  }
  *p = '\0';
  if (buf[0] != '\0') {
    start = (int)strtoll(buf, NULL, 0);
    
    while (isspace(*tail)) {
      tail++;
    }
    p = buf;
    while (!isblank(*tail) && *tail != '\n' && *tail != '\0') {
      *p++ = *tail++;
    }
    *p = '\0';
    if (buf[0] != '\0') {
      len = (int)strtoll(buf, NULL, 0);
    }
  }
  for (int addr = start; addr < start + len; ) {
    Breakpoint* bp = FindBreakpoint(interpreter, addr);
    if (bp != NULL) {
      BreakpointUninstall(interpreter, bp);
    }
    unsigned char* curr = &interpreter->memory[addr];
    void* next = Disassemble6502Instruction(interpreter->current_symbol, addr,
                              curr, stdout);
    addr += (unsigned char*)next - curr;
    if (bp != NULL) {
      BreakpointInstall(interpreter, bp);
    }
  }
}

static void SaveCommand(_6502Interpreter* interpreter, const char* cmd) {
  strcpy(interpreter->last_command, cmd);
}

static void ClearSavedCommand(_6502Interpreter* interpreter) {
  strcpy(interpreter->last_command, "");
}

static void Trace(_6502Interpreter* interpreter, const char* tail) {
  while (isspace(*tail)) {
    tail++;
  }
  if (strcmp(tail, "on\n") == 0) {
    interpreter->trace = true;
    _6502DisassemblePc(interpreter);
  } else {
    interpreter->trace = false;
  }
}

void DebuggerLoop(_6502Interpreter* interpreter) {
  for (;;) {
    char buf[256];
    printf("6502> ");
    fflush(stdout);
    fgets(buf, sizeof(buf), stdin);
    const char* p = buf;
    while (isspace(*p) && *p != '\n') {
      p++;
    }
    if (*p == '\n') {
      strcpy(buf, interpreter->last_command);
      p = buf;
      while (isspace(*p) && *p != '\n') {
        p++;
      }
    }
    enum DebugCommand cmd = ParseCommand(&p);
    switch (cmd) {
      case kDbgEmpty:
        break;
      case kDbgHelp:
        Help();
        break;
      case kDbgNone:
        printf("Unknown command\n");
        ClearSavedCommand(interpreter);
        break;
      case kDbgMemory:
        DumpMemory(interpreter, p);
        ClearSavedCommand(interpreter);
        break;
      case kDbgQuit:
        exit(0);
        break;
      case kDbgRegs:
        DumpRegs(interpreter, p);
        ClearSavedCommand(interpreter);
        break;
      case kDbgSetBreak:
         SetBreakpoint(interpreter, p);
         ClearSavedCommand(interpreter);
         break;
      case kDbgStep:
        Step(interpreter, p);
        SaveCommand(interpreter, buf);
        return;
      case kDbgStepOver:
        StepOver(interpreter, p);
        SaveCommand(interpreter, buf);
        return;
      case kDbgRun:
        if (Run(interpreter, p)) {
          ClearSavedCommand(interpreter);
          return;
        }
        break;
      case kDbgContinue:
        Continue(interpreter, p);
        SaveCommand(interpreter, buf);
        return;
      case kDbgDasm:
        Disassemble(interpreter, p);
        SaveCommand(interpreter, buf);
        break;
      case kDbgListBreak:
        ListBreakpoints(interpreter, p);
        break;
      case kDbgDelBreak:
        DeleteBreakpoint(interpreter, p);
        break;
     case kDbgTrace:
       Trace(interpreter, p);
       break;
    }
  }
}
