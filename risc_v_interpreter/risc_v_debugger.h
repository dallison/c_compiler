//
//  risc_v_debugger.h
//  risc_v_interpreter
//
//  Created by David Allison on 5/18/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#ifndef risc_v_debugger_h
#define risc_v_debugger_h

#include <stdint.h>
#include "list.h"
#include "loader.h"
#include "risc_v_interpreter.h"
#include "dstring.h"

typedef struct {
  RISCVInterpreter* interpreter;
  List breakpoints;
  int next_bp_num;
  jmp_buf main_loop;
  uint64_t entry_point;
  String last_command;
  int current_frame_id;
  SymbolScope current_function;
} RISCVDebugger;

void RISCVDebuggerInit(RISCVDebugger* debugger,
                      RISCVInterpreter* interpreter,
                       uint64_t entry_point);

void RISCVDebuggerRun(RISCVDebugger* debugger);

#endif /* risc_v_debugger_h */
