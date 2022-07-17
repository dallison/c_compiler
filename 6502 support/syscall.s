#include "vars.s"

.text

.global syscall


// Entry:
// X,Y: address of location to put result
// sp/sp+1: args:
// (sp): syscon number
// (sp), 2: first arg
syscall:
  LDA (__sp)        // Load low byte of syscall
  STA %abs(syscall_brk)   // Store in BRK instruction sequence
  TXA
  STA (__sp)          // Overwrite syscall number on stack with result address
  TYA
  LDY #1
  STA (__sp), Y

  // Invoke BRK.  The byte after the BRK is the syscall number.  The
  // word at (__sp) is the address of where to store the result.
  // The routine invoked by BRK will write its result there.
  BRK
syscall_brk:
  NOP      // Overwritten with syscall number.
  RTS

