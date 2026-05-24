//
//  arm_debugger.h
//  arm_interpreter
//

#ifndef arm_debugger_h
#define arm_debugger_h

#include <stdint.h>
#include "arm_interpreter.h"
#include "dstring.h"
#include "list.h"
#include "loader.h"

typedef struct {
  ARMInterpreter* interpreter;
  List breakpoints;
  int next_bp_num;
  jmp_buf main_loop;
  uint64_t entry_point;
  String last_command;
  int current_frame_id;
  SymbolScope current_function;
} ARMDebugger;

void ARMDebuggerInit(ARMDebugger* debugger, ARMInterpreter* interpreter,
                     uint64_t entry_point);
void ARMDebuggerRun(ARMDebugger* debugger);

#endif /* arm_debugger_h */
