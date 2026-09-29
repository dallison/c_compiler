//
//  jmpi.s
//  c_compiler
//
//  Shared indirect-call trampolines.  A call through a function pointer held
//  in zero-page register __iN used to expand inline at every call site as
//
//    jsr .thunk
//    bra .end
//  .thunk:
//    jmp (__iN)
//  .end:
//
//  which costs 8 bytes per site plus a taken branch on every return.  Each
//  call site now emits just "jsr __jmp_iN" (3 bytes) and returns straight to
//  the caller, and these 3-byte trampolines are shared by the whole program.
//

#include "vars.s"
// Functions are emitted in per-symbol ELF sections.

.global __jmp_i0
.global __jmp_i1
.global __jmp_i2
.global __jmp_i3
.global __jmp_i4
.global __jmp_i5
.global __jmp_i6
.global __jmp_i7

.section ".text.__jmp_i0", "ax", @progbits
__jmp_i0:
  JMP (__i0)
.section ".text.__jmp_i1", "ax", @progbits
__jmp_i1:
  JMP (__i1)
.section ".text.__jmp_i2", "ax", @progbits
__jmp_i2:
  JMP (__i2)
.section ".text.__jmp_i3", "ax", @progbits
__jmp_i3:
  JMP (__i3)
.section ".text.__jmp_i4", "ax", @progbits
__jmp_i4:
  JMP (__i4)
.section ".text.__jmp_i5", "ax", @progbits
__jmp_i5:
  JMP (__i5)
.section ".text.__jmp_i6", "ax", @progbits
__jmp_i6:
  JMP (__i6)
.section ".text.__jmp_i7", "ax", @progbits
__jmp_i7:
  JMP (__i7)
