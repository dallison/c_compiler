#include "vars.s"

// HIMEM for MODE 4. Called after mode(), with no arguments, so the
// caller's stack adjustment is not required.
.section ".text.park_stack", "ax", @progbits
.global park_stack
park_stack:
  LDA #0x00
  STA __sp
  LDA #0x58
  STA __sp+1
  RTS

// main never returns. This exists so _start can link its exit call.
.section ".text.exit", "ax", @progbits
.global exit
exit:
  LDX bbc_s
  TXS
  RTS
