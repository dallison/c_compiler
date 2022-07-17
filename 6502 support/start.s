#include "vars.s"
.text

.global _start

// Main entry point
_start:
  // Set up stack pointer and frame pointer.
  LDA #stack_bottom & 0xff
  STA __sp
  STA __fp
  LDA #stack_bottom >> 8
  STA __sp+1
  STA __fp+1


  // Push argv and argc onto the stack.
  // argv is at 0x200
  LDX #0
  LDY #0x2
  JSR __pushxy

  // argc is in i0
  LDX #__i0
  JSR __pushreg2

  // main returns a value.  Put it in i0.
  LDX #__i0
  LDY #0

  // Invoke main
  JSR main

  // Push exit code (in __i0) onto the runtime stack.
  LDX #__i0
  JSR __pushreg2
  JSR __pushreg2      // Push fake return address.

  // Invoke exit syscall with exit code on the runtime stack.
  BRK
  .byte sys_exit

