#include "vars.s"
.section ".text._start", "ax", @progbits

.global _start
.global exit

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
  // argv is in the reserved pages below the program image.
  LDX #0
  LDY #0x5
  JSR __pushxy

  // argc is in i0
  LDX #__i0
  JSR __pushreg2

  // main returns a value.  Put it in i0.
  LDX #__i0
  LDY #0

  // Invoke main
  JSR main

  // Call exit(main_return_value).  exit() runs atexit handlers and flushes
  // buffered stdio before performing the exit syscall, so it does not return.
  LDX #__i0
  JSR __pushreg2      // Push status argument onto the runtime stack.
  LDX #__i0
  LDY #0
  JSR exit

