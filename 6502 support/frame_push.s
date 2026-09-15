#include "vars.s"

// Functions are emitted in per-symbol ELF sections.

.global __push_var1
.global __push_var1b
.global __push_var2
.global __push_var2b
.global __push_var4
.global __push_var4b
.global __push_var8
.global __push_var8b
.global __push_arg1
.global __push_arg1b
.global __push_arg2
.global __push_arg2b
.global __push_arg4
.global __push_arg4b
.global __push_arg8
.global __push_arg8b

// Push a local variable at frame offset X,Y. The one-byte entry points clear
// the high offset byte; the `b` variants accept the full 16-bit offset.
.section ".text.__push_var1", "ax", @progbits
__push_var1:
  LDY #0
__push_var1b:
  JSR __varaddr
  BRA push_frame1

.section ".text.__push_var2", "ax", @progbits
__push_var2:
  LDY #0
__push_var2b:
  JSR __varaddr
  BRA push_frame2

.section ".text.__push_var4", "ax", @progbits
__push_var4:
  LDY #0
__push_var4b:
  JSR __varaddr
  BRA push_frame4

.section ".text.__push_var8", "ax", @progbits
__push_var8:
  LDY #0
__push_var8b:
  JSR __varaddr
  BRA push_frame8

// Push an argument at frame offset X,Y.
.section ".text.__push_var1", "ax", @progbits
__push_arg1:
  LDY #0
__push_arg1b:
  JSR __argaddr
push_frame1:
  JSR __decsp1
  LDA (__t0)
  STA (__sp)
  RTS

.section ".text.__push_var2", "ax", @progbits
__push_arg2:
  LDY #0
__push_arg2b:
  JSR __argaddr
push_frame2:
  JSR __decsp2
  LDA (__t0)
  STA (__sp)
  LDY #1
  LDA (__t0),Y
  STA (__sp),Y
  RTS

.section ".text.__push_var4", "ax", @progbits
__push_arg4:
  LDY #0
__push_arg4b:
  JSR __argaddr
push_frame4:
  JSR __decsp4
  LDY #3
push_frame4_loop:
  LDA (__t0),Y
  STA (__sp),Y
  DEY
  BPL push_frame4_loop
  RTS

.section ".text.__push_var8", "ax", @progbits
__push_arg8:
  LDY #0
__push_arg8b:
  JSR __argaddr
push_frame8:
  JSR __decsp8
  LDY #7
push_frame8_loop:
  LDA (__t0),Y
  STA (__sp),Y
  DEY
  BPL push_frame8_loop
  RTS
