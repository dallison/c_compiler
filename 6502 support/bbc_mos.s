#include "vars.s"

// C wrappers for the BBC MOS entry points. Register state lives in the
// mailbox owned by bbc_syscall.s. bbc_os_call enters MOS with A, X, and Y.
// MOS does not use the language zero page, so these calls leave it in place.
//
// A struct os_regs result is returned through the pointer at (__sp).
// Void calls leave argument cleanup to the caller.

.set OSRDSC 0xffb9
.set OSEVEN 0xffbf
.set GSINIT 0xffc2
.set GSREAD 0xffc5
.set NVRDCH 0xffc8
.set NVWRCH 0xffcb
.set OSFIND 0xffce
.set OSGBPB 0xffd1
.set OSBPUT 0xffd4
.set OSBGET 0xffd7
.set OSARGS 0xffda
.set OSFILE 0xffdd
.set OSRDCH 0xffe0
.set OSASCI 0xffe3
.set OSNEWL 0xffe7
.set OSWRCR 0xffec
.set OSWRCH 0xffee
.set OSWORD 0xfff1
.set OSBYTE 0xfff4
.set OSCLI  0xfff7
.set FSCV   0x021e
.set GSPTR  0x00f2
.set ARGZP  0x0070

.comm bbc_blk, 2

.section ".text.bbc_store_regs", "ax", @progbits
.global bbc_store_regs
bbc_store_regs:
  LDY #0
  LDA (__sp), Y
  STA __t2
  INY
  LDA (__sp), Y
  STA __t2+1
  LDY #0
  LDA os_a
  STA (__t2), Y
  INY
  LDA os_x
  STA (__t2), Y
  INY
  LDA os_y
  STA (__t2), Y
  INY
  LDA os_p
  STA (__t2), Y
  RTS

// sret at 0, then A, X, Y as 2-byte stack slots.
.section ".text.bbc_load_axy", "ax", @progbits
.global bbc_load_axy
bbc_load_axy:
  LDY #2
  LDA (__sp), Y
  STA os_a
  LDY #4
  LDA (__sp), Y
  STA os_x
  LDY #6
  LDA (__sp), Y
  STA os_y
  RTS

.section ".text.osbyte", "ax", @progbits
.global osbyte
osbyte:
  JSR bbc_load_axy
  LDA #%lo(OSBYTE)
  STA os_target
  LDA #%hi(OSBYTE)
  STA os_target+1
  JSR bbc_os_call
  JMP bbc_store_regs

.section ".text.osevent", "ax", @progbits
.global osevent
osevent:
  JSR bbc_load_axy
  LDA #%lo(OSEVEN)
  STA os_target
  LDA #%hi(OSEVEN)
  STA os_target+1
  JSR bbc_os_call
  JMP bbc_store_regs

.section ".text.osfsc", "ax", @progbits
.global osfsc
osfsc:
  JSR bbc_load_axy
  LDA FSCV
  STA os_target
  LDA FSCV+1
  STA os_target+1
  JSR bbc_os_call
  JMP bbc_store_regs

.section ".text.osword", "ax", @progbits
.global osword
osword:
  LDY #0
  LDA (__sp), Y
  STA os_a
  LDY #2
  LDA (__sp), Y
  STA os_x
  INY
  LDA (__sp), Y
  STA os_y
  LDA #%lo(OSWORD)
  STA os_target
  LDA #%hi(OSWORD)
  STA os_target+1
  JMP bbc_os_call

.section ".text.oswrch", "ax", @progbits
.global oswrch
oswrch:
  LDY #0
  LDA (__sp), Y
  STA os_a
  STZ os_x
  STZ os_y
  LDA #%lo(OSWRCH)
  STA os_target
  LDA #%hi(OSWRCH)
  STA os_target+1
  JMP bbc_os_call

.section ".text.osasci", "ax", @progbits
.global osasci
osasci:
  LDY #0
  LDA (__sp), Y
  STA os_a
  STZ os_x
  STZ os_y
  LDA #%lo(OSASCI)
  STA os_target
  LDA #%hi(OSASCI)
  STA os_target+1
  JMP bbc_os_call

.section ".text.nvwrch", "ax", @progbits
.global nvwrch
nvwrch:
  LDY #0
  LDA (__sp), Y
  STA os_a
  STZ os_x
  STZ os_y
  LDA #%lo(NVWRCH)
  STA os_target
  LDA #%hi(NVWRCH)
  STA os_target+1
  JMP bbc_os_call

.section ".text.osnewl", "ax", @progbits
.global osnewl
osnewl:
  STZ os_a
  STZ os_x
  STZ os_y
  LDA #%lo(OSNEWL)
  STA os_target
  LDA #%hi(OSNEWL)
  STA os_target+1
  JMP bbc_os_call

.section ".text.oswrcr", "ax", @progbits
.global oswrcr
oswrcr:
  STZ os_a
  STZ os_x
  STZ os_y
  LDA #%lo(OSWRCR)
  STA os_target
  LDA #%hi(OSWRCR)
  STA os_target+1
  JMP bbc_os_call

.section ".text.osrdch", "ax", @progbits
.global osrdch
osrdch:
  STZ os_a
  STZ os_x
  STZ os_y
  LDA #%lo(OSRDCH)
  STA os_target
  LDA #%hi(OSRDCH)
  STA os_target+1
  JSR bbc_os_call
  JMP bbc_store_regs

.section ".text.nvrdch", "ax", @progbits
.global nvrdch
nvrdch:
  STZ os_a
  STZ os_x
  STZ os_y
  LDA #%lo(NVRDCH)
  STA os_target
  LDA #%hi(NVRDCH)
  STA os_target+1
  JSR bbc_os_call
  JMP bbc_store_regs

.section ".text.oscli", "ax", @progbits
.global oscli
oscli:
  STZ os_a
  LDY #0
  LDA (__sp), Y
  STA os_x
  INY
  LDA (__sp), Y
  STA os_y
  LDA #%lo(OSCLI)
  STA os_target
  LDA #%hi(OSCLI)
  STA os_target+1
  JMP bbc_os_call

.section ".text.osfile", "ax", @progbits
.global osfile
osfile:
  LDY #2
  LDA (__sp), Y
  STA os_a
  LDY #4
  LDA (__sp), Y
  STA os_x
  INY
  LDA (__sp), Y
  STA os_y
  LDA #%lo(OSFILE)
  STA os_target
  LDA #%hi(OSFILE)
  STA os_target+1
  JSR bbc_os_call
  JMP bbc_store_regs

.section ".text.osgbpb", "ax", @progbits
.global osgbpb
osgbpb:
  LDY #2
  LDA (__sp), Y
  STA os_a
  LDY #4
  LDA (__sp), Y
  STA os_x
  INY
  LDA (__sp), Y
  STA os_y
  LDA #%lo(OSGBPB)
  STA os_target
  LDA #%hi(OSGBPB)
  STA os_target+1
  JSR bbc_os_call
  JMP bbc_store_regs

.section ".text.osbget", "ax", @progbits
.global osbget
osbget:
  STZ os_a
  STZ os_x
  LDY #2
  LDA (__sp), Y
  STA os_y
  LDA #%lo(OSBGET)
  STA os_target
  LDA #%hi(OSBGET)
  STA os_target+1
  JSR bbc_os_call
  JMP bbc_store_regs

.section ".text.osbput", "ax", @progbits
.global osbput
osbput:
  LDY #0
  LDA (__sp), Y
  STA os_a
  STZ os_x
  LDY #2
  LDA (__sp), Y
  STA os_y
  LDA #%lo(OSBPUT)
  STA os_target
  LDA #%hi(OSBPUT)
  STA os_target+1
  JMP bbc_os_call

// a == 0 closes the handle in Y. Any other A opens the CR-terminated name.
.section ".text.osfind", "ax", @progbits
.global osfind
osfind:
  LDY #2
  LDA (__sp), Y
  STA os_a
  BNE osfind_open
  STZ os_x
  LDY #4
  LDA (__sp), Y
  STA os_y
  JMP osfind_go
osfind_open:
  LDY #6
  LDA (__sp), Y
  STA os_x
  INY
  LDA (__sp), Y
  STA os_y
osfind_go:
  LDA #%lo(OSFIND)
  STA os_target
  LDA #%hi(OSFIND)
  STA os_target+1
  JSR bbc_os_call
  JMP bbc_store_regs

// (&F2) is the GSINIT/GSREAD string pointer. It sits in MOS workspace, so a
// later gsread sees the same string.
.section ".text.gsinit", "ax", @progbits
.global gsinit
gsinit:
  LDY #2
  LDA (__sp), Y
  STA os_a
  STZ os_x
  STZ os_y
  LDY #4
  LDA (__sp), Y
  STA GSPTR
  INY
  LDA (__sp), Y
  STA GSPTR+1
  LDA #%lo(GSINIT)
  STA os_target
  LDA #%hi(GSINIT)
  STA os_target+1
  JSR bbc_os_call
  JMP bbc_store_regs

.section ".text.gsread", "ax", @progbits
.global gsread
gsread:
  STZ os_a
  STZ os_x
  LDY #2
  LDA (__sp), Y
  STA os_y
  LDA #%lo(GSREAD)
  STA os_target
  LDA #%hi(GSREAD)
  STA os_target+1
  JSR bbc_os_call
  JMP bbc_store_regs

.section ".text.osrdsc", "ax", @progbits
.global osrdsc
osrdsc:
  STZ os_a
  LDY #2
  LDA (__sp), Y
  STA os_x
  LDY #4
  LDA (__sp), Y
  STA os_y
  LDA #%lo(OSRDSC)
  STA os_target
  LDA #%hi(OSRDSC)
  STA os_target+1
  JSR bbc_os_call
  JMP bbc_store_regs

// MOS requires the 4-byte OSARGS block in zero page. Stage it at ARGZP,
// which is math scratch.
.section ".text.osargs", "ax", @progbits
.global osargs
osargs:
  LDY #2
  LDA (__sp), Y
  STA os_a
  LDY #4
  LDA (__sp), Y
  STA os_y
  LDY #6
  LDA (__sp), Y
  STA bbc_blk
  INY
  LDA (__sp), Y
  STA bbc_blk+1
  LDA bbc_blk
  STA __t2
  LDA bbc_blk+1
  STA __t2+1
  LDY #0
osargs_in:
  LDA (__t2), Y
  STA ARGZP, Y
  INY
  CPY #4
  BNE osargs_in
  LDA #ARGZP
  STA os_x
  LDA #%lo(OSARGS)
  STA os_target
  LDA #%hi(OSARGS)
  STA os_target+1
  JSR bbc_os_invoke
  LDA bbc_blk
  STA __t2
  LDA bbc_blk+1
  STA __t2+1
  LDY #0
osargs_out:
  LDA ARGZP, Y
  STA (__t2), Y
  INY
  CPY #4
  BNE osargs_out
  JMP bbc_store_regs
