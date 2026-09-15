#include "vars.s"

.section ".text.syscall", "ax", @progbits

.global syscall


// Entry:
// X,Y: address of location to put result
// sp/sp+1: args:
// (sp): syscon number
// (sp), 2: first arg
syscall:
  LDA (__sp)        // Load low byte of syscall
  STA %abs(syscall_code)   // Store in LDA immediate instruction
  TXA
  STA (__sp)          // Overwrite syscall number on stack with result address
  TYA
  LDY #1
  STA (__sp), Y

  // Load the patched syscall number into A and invoke the interpreter's
  // custom software break. The host writes the four-byte long result through
  // the address now stored at (__sp).
  .byte 0xa9       // LDA immediate
syscall_code:
  .byte 0          // Overwritten with syscall number.
  .byte 0xef       // Interpreter syscall.
  RTS

