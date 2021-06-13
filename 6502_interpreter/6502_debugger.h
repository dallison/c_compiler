//
//  6502_debugger.h
//  6502_interpreter
//
//  Created by David Allison on 6/4/21.
//  Copyright © 2021 David Allison. All rights reserved.
//

#ifndef _502_debugger_h
#define _502_debugger_h

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "6502_disassembler.h"

struct _6502Interpreter;

typedef struct  {
  int num;
  int addr;
  char old_value;
  bool enabled;
  bool is_temp;
} Breakpoint;

Breakpoint* NewBreakpoint(int num, int addr, char old_value, bool temp);
void BreakpointDestruct(Breakpoint* bp);
void BreakpointDelete(Breakpoint* bp);
void BreakpointInstall(struct _6502Interpreter* interpreter, Breakpoint* bp);
void BreakpointUninstall(struct _6502Interpreter* interpreter, Breakpoint* bp);

Breakpoint* FindBreakpoint(struct _6502Interpreter* interpreter, int addr);
void DebuggerLoop(struct _6502Interpreter* interpreter);
void RemoveBreakpoint(struct _6502Interpreter* interpreter, Breakpoint* bp);

#endif /* _502_debugger_h */
