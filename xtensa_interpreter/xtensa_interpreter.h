//
//  xtensa_interpreter.h
//  c_compiler
//

#ifndef xtensa_interpreter_h
#define xtensa_interpreter_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "loader.h"

#define XTENSA_HOSTED_STACK_BASE 0x3ff00000u
#define XTENSA_HOSTED_STACK_SIZE (512u * 1024u)
#define XTENSA_MAX_CALL_DEPTH 4096

typedef struct {
  uint32_t registers[64];
  uint32_t return_pc;
  uint8_t window_base;
} XtensaCallFrame;

typedef struct {
  Loader* loader;
  uint32_t aregs[64];
  uint32_t pc;
  uint32_t sar;
  uint32_t ps;
  uint32_t lbeg;
  uint32_t lend;
  uint32_t lcount;
  uint8_t window_base;
  uint8_t call_increment;
  uint64_t window_start;
  XtensaCallFrame calls[XTENSA_MAX_CALL_DEPTH];
  size_t call_depth;
  uint8_t* stack;
  bool trace;
  bool running;
  int exit_code;
  uint64_t steps;
} XtensaInterpreter;

bool XtensaInterpreterInit(XtensaInterpreter* interpreter, Loader* loader,
                           uint32_t entry, bool trace);
void XtensaInterpreterDestruct(XtensaInterpreter* interpreter);
bool XtensaInterpreterStep(XtensaInterpreter* interpreter);
int XtensaInterpreterRun(XtensaInterpreter* interpreter);

#endif /* xtensa_interpreter_h */
