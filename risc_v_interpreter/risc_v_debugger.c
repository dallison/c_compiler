//
//  risc_v_debugger.c
//  risc_v_interpreter
//
//  Created by David Allison on 5/18/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#include "risc_v_debugger.h"
#include "risc_v_disassembler.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>

typedef struct {
  ListElement header;
  int num;
  uint64_t address;
  bool is_temp;
  int32_t instruction;  // Instruction replaced.
} Breakpoint;

static void BreakpointInit(Breakpoint* bp, int num, uint64_t address, bool is_temp) {
  ListElementInit(&bp->header);
  bp->num = num;
  bp->address = address;
  bp->is_temp = is_temp;
}

static Breakpoint* NewBreakpoint(int num, uint64_t address, bool is_temp) {
  Breakpoint* bp = malloc(sizeof(Breakpoint));
  BreakpointInit(bp, num, address, is_temp);
  return bp;
}

static Breakpoint* FindBreakpointAtAddress(RISCVDebugger* debugger, uint64_t addr) {
  for (ListElement* element = debugger->breakpoints.first; element != NULL; element = element->next) {
    Breakpoint* bp = (Breakpoint*)element;
    if (bp->address == addr) {
      return bp;
    }
  }
  return NULL;
}

static Breakpoint* FindBreakpointAtCurrentPC(RISCVDebugger* debugger) {
  uint64_t pc = debugger->interpreter->pc;
  return FindBreakpointAtAddress(debugger, pc);
}


static Breakpoint* FindBreakpointByNumber(RISCVDebugger* debugger, int bpnum) {
  for (ListElement* element = debugger->breakpoints.first; element != NULL; element = element->next) {
    Breakpoint* bp = (Breakpoint*)element;
    if (bp->num == bpnum) {
      return bp;
    }
  }
  return NULL;
}

static void RestoreBreakpointInstruction(Breakpoint* bp) {
  *(int32_t*)bp->address = bp->instruction;
}

static void InsertBreakpoint(Breakpoint* bp) {
  bp->instruction = *((int32_t*)bp->address);
  *(int32_t*)bp->address = (1 << 20) | 0x73;     // Insert ebreak instruction.
}

static void CreateBreakpoint(RISCVDebugger* debugger, int64_t addr, bool is_temp) {
  Breakpoint* bp = FindBreakpointAtAddress(debugger, addr);
  if (bp != NULL) {
    bp->is_temp = is_temp;
    return;
  }
  bp = NewBreakpoint(debugger->next_bp_num++, addr, is_temp);
  ListAppend(&debugger->breakpoints, &bp->header);
  InsertBreakpoint(bp);
  if (!is_temp) {
    printf("Breakpoint %d set at address 0x%" PRIx64 "\n", bp->num, addr);
  }
}

void RISCVDebuggerInit(RISCVDebugger* debugger,
                       struct RISCVInterpreter* interpreter,
                       uint64_t entry_point) {
  debugger->interpreter = interpreter;
  debugger->next_bp_num = 1;
  debugger->entry_point = entry_point;
  debugger->current_frame_id = 0;
  ListInit(&debugger->breakpoints);
  StringInit(&debugger->last_command, NULL);
  CreateBreakpoint(debugger, entry_point, true);
}

static void ShowLocation(RISCVDebugger* debugger, uint64_t pc, SymbolScope* symbol, int frame_id) {
  printf("%-3s", frame_id == debugger->current_frame_id ? "=>" : "");
  printf("%-3d 0x%016" PRIx64 " %s\n", frame_id, pc, symbol == NULL ? "" : symbol->name);
}


static void BreakpointHit(RISCVDebugger* debugger) {
  Breakpoint* bp = FindBreakpointAtCurrentPC(debugger);
  assert(bp != NULL);

  if (bp->is_temp) {
    RestoreBreakpointInstruction(bp);
    ListDeleteElement(&debugger->breakpoints, &bp->header);
    return;
  }
  debugger->current_frame_id = 0;
  SymbolScope curr;
  bool ok = LoaderFindSymbol(debugger->interpreter->loader,
                                       debugger->interpreter->pc, &curr);
  ShowLocation(debugger, debugger->interpreter->pc, ok ? &curr : NULL, 0);
  longjmp(debugger->main_loop, 1);
}

void RISCVDebuggerCycle(RISCVDebugger* debugger) {
  for (;;) {
    if (setjmp(debugger->interpreter->debugger) == 0) {
      RISCVInterpreterCycle(debugger->interpreter);
      return;
    } else {
      // Breakpoint hit.
      BreakpointHit(debugger);
    }
  }
}

static uint64_t FindSymbolAddress(RISCVDebugger* debugger, const char* name) {
  return LoaderLookupSymbol(debugger->interpreter->loader, name);
}

typedef enum {
  kCommandEof,
  kCommandRun,
  kCommandContinue,
  kCommandSetBreak,
  kCommandDelBreak,
  kCommandStep,
  kCommandNext,
  kCommandPrint,
  kCommandDump,
  kCommandUnknown,
  kCommandRegs,
  kCommandDasm,
  kCommandHelp,
  kCommandUp,
  kCommandDown,
  kCommandBacktrace,
  kCommandTrace,
  kCommandQuit,
  kCommandEmpty,
} Command;

static struct {
  const char* spelling;
  Command command;
  const char* help;
} commands[] = {
  {"run", kCommandRun, "Run from beginning"},
  {"cont", kCommandContinue, "Continue execution"},
  {"c", kCommandContinue, "Continue execution"},
  {"b", kCommandSetBreak, "Set breakpoint (*addr or symbol)"},
  {"break", kCommandSetBreak, "Set breakpoint (*addr or symbol)"},
  {"del", kCommandDelBreak, "Delete breakpoint number"},
  {"s", kCommandStep, "Single step instruction"},
  {"step", kCommandStep, "Single step instruction"},
  {"n", kCommandNext, "Single step instruction over calls"},
  {"next", kCommandNext, "Single step instruction over calls"},
  {"p", kCommandPrint, "Print expression"},
  {"x", kCommandDump, "Dump memory"},
  {"regs", kCommandRegs, "Dump registers"},
  {"dasm", kCommandDasm, "Disassemble"},
  {"up", kCommandUp, "Up one frame"},
  {"down", kCommandDown, "Down one frame"},
  {"bt", kCommandBacktrace, "Stack trace"},
  {"where", kCommandBacktrace, "Stack trace"},
  {"trace", kCommandTrace, "Switch instrution tracing on or off"},
  {"quit", kCommandQuit, "Quit"},
  {"q", kCommandQuit, "Quit"},
  {"help", kCommandHelp, "Help"},
  {NULL, kCommandEof, NULL},
};

static Command GetCommand(RISCVDebugger* debugger,
                          char* buf, int buflen, char** end) {
  printf("riscv> ");
  fflush(stdout);
  char* s = fgets(buf, buflen, stdin);
  if (s == NULL) {
    return kCommandEof;
  }
  char* p = s;
  // Skip leading spaces.
  while (*p != '\0' && isspace(*p)) {
    p++;
  }
  // Trim spaces off end.
  char* e = p + strlen(p) - 1;
  while (isspace(*e)) {
    *e-- = '\0';
  }
  if (*p == '\0') {
    // Empty command.
    if (debugger->last_command.length > 0) {
      strncpy(buf, debugger->last_command.value, buflen);
      p = buf;
    }
  }
  char cmdbuf[256];
  s = cmdbuf;
  while (*p != '\0' && isalpha(*p)) {
    *s++ = *p++;
  }
  *s = 0;
  if (cmdbuf[0] == '\0') {
    return kCommandEmpty;
  }
  // Skip trailing spaces.
  while (*p != '\0' && isspace(*p)) {
    p++;
  }
  *end = p;
  for (size_t i = 0; commands[i].spelling != NULL; i++) {
    if (strcmp(commands[i].spelling, cmdbuf) == 0) {
      return commands[i].command;
    }
  }
  return kCommandUnknown;
}

static void Run(RISCVDebugger* debugger, char* tail) {
  if (setjmp(debugger->main_loop) == 0) {
    debugger->interpreter->num_steps = -1;
    RISCVDebuggerCycle(debugger);
  }
}

static void Continue(RISCVDebugger* debugger, char* tail) {
  if (setjmp(debugger->main_loop) == 0) {
    Breakpoint* bp = FindBreakpointAtCurrentPC(debugger);
    if (bp != NULL) {
      // Continuing from a breakpoint.  Replace breakpoint, run one cycle
      // and restore breakpoint.
      RestoreBreakpointInstruction(bp);
      debugger->interpreter->num_steps = 1;
      RISCVDebuggerCycle(debugger);
      InsertBreakpoint(bp);
    }
    // Now continue execution.
    debugger->interpreter->num_steps = -1;
    RISCVDebuggerCycle(debugger);
  }
}

static void Step(RISCVDebugger* debugger, char* tail) {
  if (setjmp(debugger->main_loop) == 0) {
    debugger->interpreter->num_steps = 1;
    Breakpoint* bp = FindBreakpointAtCurrentPC(debugger);
    if (bp != NULL) {
       // Replace breakpoint, run one cycle and restore breakpoint.
       RestoreBreakpointInstruction(bp);
       RISCVDebuggerCycle(debugger);
       InsertBreakpoint(bp);
       return;
    }
    RISCVDebuggerCycle(debugger);
  }
}

static void Next(RISCVDebugger* debugger, char* tail) {
  if (setjmp(debugger->main_loop) == 0) {
    debugger->interpreter->num_steps = 1;
    Breakpoint* bp = FindBreakpointAtCurrentPC(debugger);
    if (bp != NULL) {
       // Replace breakpoint, run one cycle and restore breakpoint.
       RestoreBreakpointInstruction(bp);
       RISCVDebuggerCycle(debugger);
       InsertBreakpoint(bp);
       return;
    }
    RISCVDebuggerCycle(debugger);
  }
}

static void SetBreakpoint(RISCVDebugger* debugger, char* tail) {
  char symbol_name[256];
  char* s = symbol_name;
  if (*tail == '*') {
    // By address.
    tail++;
    uint64_t addr = strtoull(tail, NULL, 0);
    if (addr == 0) {
      printf("Invalid breakpoint address\n");
      return;
    }
    CreateBreakpoint(debugger, addr, false);
    return;
  }
  while (*tail != '\0' && !isspace(*tail)) {
    *s++ = *tail++;
  }
  *s = 0;
  uint64_t addr = FindSymbolAddress(debugger, symbol_name);
  if (addr == 0) {
    printf("Unknown symbol %s", symbol_name);
    return;
  }
  CreateBreakpoint(debugger, addr, false);
}

static void DelBreakpoint(RISCVDebugger* debugger, char* tail) {
  char* end;
  int bpnum = (int)strtoll(tail, &end, 0);
  if (bpnum == 0) {
    printf("No such breakpoint\n");
    return;
  }
  Breakpoint* bp = FindBreakpointByNumber(debugger, bpnum);
  if (bp == NULL) {
    printf("No such breakpoint %d\n", bpnum);
    return;
  }
  ListDeleteElement(&debugger->breakpoints, &bp->header);
}

static void Print(RISCVDebugger* debugger, char* tail) {
}

static void Dump(RISCVDebugger* debugger, char* tail) {
  int size = 10;
  char* p = tail;
  if (*tail == '/') {
    tail++;
    size = (int)strtoll(tail, &p, 10);
    if (size == 0) {
      size = 10;
    }
  }
  tail = p;
  int64_t addr = strtoll(tail, NULL, 0);
  if (addr == 0) {
    return;
  }
  
  char buf[16];
  const char* caddr = (const char*)(addr);
  const char* endaddr = caddr + size;
  int len = size;
  while (len > 0) {
    printf("%p  ", caddr);
    for (int i = 0; i < 16; i++) {
      if (caddr > endaddr) {
        buf[i] = 0xff;
      } else {
        buf[i] = *caddr++;
      }
      printf("%02X ", buf[i] & 0xff);
    }
    printf("  ");
    for (int i = 0; i < 16; i++) {
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


static void ShowRegs(RISCVDebugger* debugger, char* tail) {
  RISCVInterpreterDumpRegisters(debugger->interpreter);
}

static void Disassemble(RISCVDebugger* debugger, char* tail) {
  char* p;
  int64_t start = strtoll(tail, &p, 0);
  if (start == 0) {
    start = debugger->interpreter->pc;
  }
  int64_t end = strtoll(p, NULL, 0);
  if (end == 0) {
    end = start + 100;
  }
  int64_t addr = start;
  while (addr < end) {
    DisassembleRiscVInstruction(debugger->interpreter, (void*)addr, stdout);
    addr += 4;
  }
}

static void Move(RISCVDebugger* debugger, char* tail, bool up) {

}


static void Backtrace(RISCVDebugger* debugger, char* tail) {
  SymbolScope curr;
  bool found = LoaderFindSymbol(debugger->interpreter->loader,
                                       debugger->interpreter->pc, &curr);
  ShowLocation(debugger, debugger->interpreter->pc, found ? &curr: NULL, 0);
  uint64_t fp = debugger->interpreter->iregs[RISC_V_REG_s0];
  int frame_id = 1;
  uint64_t top_frame_start = (uint64_t)debugger->interpreter->startup_code;
  uint64_t top_frame_end = (uint64_t)debugger->interpreter->startup_code +
        sizeof(debugger->interpreter->startup_code);

  for (;;) {
    uint64_t ra = *(uint64_t*)(fp - 8);
    if (ra >= top_frame_start && ra < top_frame_end) {
      break;
    }
    found = LoaderFindSymbol(debugger->interpreter->loader, ra, &curr);
    ShowLocation(debugger, ra, found ? &curr: NULL, frame_id);
    frame_id++;
    fp = *(uint64_t*)(fp - 16);
  }
}

static void Trace(RISCVDebugger* debugger, char* tail) {
  if (strcmp(tail, "on") == 0) {
    debugger->interpreter->trace_instructions = true;
  } else if (strcmp(tail, "off") == 0){
    debugger->interpreter->trace_instructions = false;
  } else {
    printf("Instruction tracing is %s\n", debugger->interpreter->trace_instructions ? "on" : "off");
  }
}

static void Help(RISCVDebugger* debugger, char* tail) {
  for (size_t i = 0; commands[i].spelling != NULL; i++) {
    printf("%-10s %s\n", commands[i].spelling, commands[i].help);
  }
}

static void RecordCommand(RISCVDebugger* debugger, const char* command) {
  StringSet(&debugger->last_command, command);
}

static void ClearCommand(RISCVDebugger* debugger) {
  StringSet(&debugger->last_command, "");
}

void RISCVDebuggerRun(RISCVDebugger* debugger) {
  for (;;) {
    char buf[256];
    char* tail;
    Command cmd = GetCommand(debugger, buf, sizeof(buf), &tail);
    if (setjmp(debugger->main_loop) == 0) {
      switch (cmd) {
        case kCommandEof:
          return;
        case kCommandRun:
          Run(debugger, tail);
          RecordCommand(debugger, buf);
          break;
        case kCommandContinue:
          Continue(debugger, tail);
          RecordCommand(debugger, buf);
          break;
        case kCommandSetBreak:
          SetBreakpoint(debugger, tail);
          ClearCommand(debugger);
          break;
        case kCommandDelBreak:
          DelBreakpoint(debugger, tail);
          ClearCommand(debugger);
          break;
        case kCommandStep:
          Step(debugger, tail);
          RecordCommand(debugger, buf);
          break;
        case kCommandNext:
          Next(debugger, tail);
          RecordCommand(debugger, buf);
          break;
        case kCommandPrint:
          Print(debugger, tail);
          RecordCommand(debugger, buf);
          break;
        case kCommandDump:
          Dump(debugger, tail);
          RecordCommand(debugger, buf);
          break;
        case kCommandRegs:
          ShowRegs(debugger, tail);
          RecordCommand(debugger, buf);
          break;
        case kCommandDasm:
           Disassemble(debugger, tail);
           RecordCommand(debugger, buf);
           break;
        case kCommandHelp:
           Help(debugger, tail);
           ClearCommand(debugger);
          break;
        case kCommandUp:
         case kCommandDown:
           Move(debugger, tail, cmd == kCommandUp);
           RecordCommand(debugger, buf);
          break;
        case kCommandBacktrace:
          Backtrace(debugger, tail);
          break;
        case kCommandTrace:
          Trace(debugger, tail);
          break;
        case kCommandQuit:
          exit(0);
          break;
        case kCommandEmpty:
          break;
        case kCommandUnknown:
          printf("Unknown command %s\n", buf);
          break;
      }
    }
  }
}

