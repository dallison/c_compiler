#include "vars.s"

// BASIC-shaped wrappers around the VDU driver. OSWRCH, OSBYTE, and OSWORD
// leave the language zero page alone, so these call the entry points directly.
// Void functions leave the software-stack arguments for the caller to drop.

.set VDU_OSWRCH 0xffee
.set VDU_OSWORD 0xfff1
.set VDU_OSBYTE 0xfff4

.comm bbc_vlen, 2
.comm bbc_point, 5

.section ".text.bbc_vdu", "ax", @progbits
.global bbc_vdu
bbc_vdu:
  JSR VDU_OSWRCH
  RTS

// Y is the software-stack offset of a 16-bit value. Sends it little-endian.
.section ".text.bbc_vdu_pair", "ax", @progbits
.global bbc_vdu_pair
bbc_vdu_pair:
  LDA (__sp), Y
  PHA
  TYA
  PHA
  TSX
  LDA 0x102, X
  JSR bbc_vdu
  PLA
  TAY
  PLA
  INY
  LDA (__sp), Y
  JMP bbc_vdu

// A is the PLOT code. x is at (__sp), y is at (__sp)+2.
.section ".text.bbc_plot_xy", "ax", @progbits
.global bbc_plot_xy
bbc_plot_xy:
  PHA
  LDA #25
  JSR bbc_vdu
  PLA
  JSR bbc_vdu
  LDY #0
  JSR bbc_vdu_pair
  LDY #2
  JMP bbc_vdu_pair

// A is a byte. __t2 points at the int result. The high byte is zero.
.section ".text.bbc_u8_result", "ax", @progbits
.global bbc_u8_result
bbc_u8_result:
  LDY #0
  STA (__t2), Y
  LDA #0
  INY
  STA (__t2), Y
  RTS

.section ".text.mode", "ax", @progbits
.global mode
mode:
  LDA #22
  JSR bbc_vdu
  LDY #0
  LDA (__sp), Y
  JMP bbc_vdu

.section ".text.colour", "ax", @progbits
.global colour
.global color
colour:
color:
  LDA #17
  JSR bbc_vdu
  LDY #0
  LDA (__sp), Y
  JMP bbc_vdu

.section ".text.gcol", "ax", @progbits
.global gcol
gcol:
  LDA #18
  JSR bbc_vdu
  LDY #0
  LDA (__sp), Y
  JSR bbc_vdu
  LDY #2
  LDA (__sp), Y
  JMP bbc_vdu

.section ".text.palette", "ax", @progbits
.global palette
palette:
  LDA #19
  JSR bbc_vdu
  LDY #0
  LDA (__sp), Y
  JSR bbc_vdu
  LDY #2
  LDA (__sp), Y
  JSR bbc_vdu
  LDA #0
  JSR bbc_vdu
  JSR bbc_vdu
  JMP bbc_vdu

.section ".text.plot", "ax", @progbits
.global plot
plot:
  LDA #25
  JSR bbc_vdu
  LDY #0
  LDA (__sp), Y
  JSR bbc_vdu
  LDY #2
  JSR bbc_vdu_pair
  LDY #4
  JMP bbc_vdu_pair

.section ".text.move", "ax", @progbits
.global move
move:
  LDA #4
  JMP bbc_plot_xy

.section ".text.draw", "ax", @progbits
.global draw
draw:
  LDA #5
  JMP bbc_plot_xy

.section ".text.dot", "ax", @progbits
.global dot
dot:
  LDA #69
  JMP bbc_plot_xy

.section ".text.triangle", "ax", @progbits
.global triangle
triangle:
  LDA #85
  JMP bbc_plot_xy

.section ".text.point", "ax", @progbits
.global point
point:
  STX __t2
  STY __t2+1
  LDY #0
  LDA (__sp), Y
  STA bbc_point
  INY
  LDA (__sp), Y
  STA bbc_point+1
  LDY #2
  LDA (__sp), Y
  STA bbc_point+2
  INY
  LDA (__sp), Y
  STA bbc_point+3
  LDA #9
  LDX #%lo(bbc_point)
  LDY #%hi(bbc_point)
  JSR VDU_OSWORD
  LDA bbc_point+4
  CMP #0xff
  BNE point_colour
  LDA #0xff
  LDY #0
  STA (__t2), Y
  INY
  STA (__t2), Y
  JMP point_pop
point_colour:
  JSR bbc_u8_result
point_pop:
  CLC
  LDA __sp
  ADC #4
  STA __sp
  LDA __sp+1
  ADC #0
  STA __sp+1
  RTS

.section ".text.cls", "ax", @progbits
.global cls
cls:
  LDA #12
  JMP bbc_vdu

.section ".text.clg", "ax", @progbits
.global clg
clg:
  LDA #16
  JMP bbc_vdu

.section ".text.origin", "ax", @progbits
.global origin
origin:
  LDA #29
  JSR bbc_vdu
  LDY #0
  JSR bbc_vdu_pair
  LDY #2
  JMP bbc_vdu_pair

.section ".text.graphics_window", "ax", @progbits
.global graphics_window
graphics_window:
  LDA #24
  JSR bbc_vdu
  LDY #0
  JSR bbc_vdu_pair
  LDY #2
  JSR bbc_vdu_pair
  LDY #4
  JSR bbc_vdu_pair
  LDY #6
  JMP bbc_vdu_pair

.section ".text.text_window", "ax", @progbits
.global text_window
text_window:
  LDA #28
  JSR bbc_vdu
  LDY #0
  LDA (__sp), Y
  JSR bbc_vdu
  LDY #2
  LDA (__sp), Y
  JSR bbc_vdu
  LDY #4
  LDA (__sp), Y
  JSR bbc_vdu
  LDY #6
  LDA (__sp), Y
  JMP bbc_vdu

.section ".text.tab", "ax", @progbits
.global tab
tab:
  LDA #31
  JSR bbc_vdu
  LDY #0
  LDA (__sp), Y
  JSR bbc_vdu
  LDY #2
  LDA (__sp), Y
  JMP bbc_vdu

.section ".text.home", "ax", @progbits
.global home
home:
  LDA #30
  JMP bbc_vdu

.section ".text.pos", "ax", @progbits
.global pos
pos:
  STX __t2
  STY __t2+1
  LDA #134
  JSR VDU_OSBYTE
  TXA
  JMP bbc_u8_result

.section ".text.vpos", "ax", @progbits
.global vpos
vpos:
  STX __t2
  STY __t2+1
  LDA #134
  JSR VDU_OSBYTE
  TYA
  JMP bbc_u8_result

.section ".text.cursor", "ax", @progbits
.global cursor
cursor:
  LDA #23
  JSR bbc_vdu
  LDA #1
  JSR bbc_vdu
  LDY #0
  LDA (__sp), Y
  JSR bbc_vdu
  ; VDU 23 takes nine parameter bytes: 1, on/off, then seven zeros.
  LDX #7
cursor_pad:
  LDA #0
  JSR bbc_vdu
  DEX
  BNE cursor_pad
  RTS

.section ".text.text_cursor", "ax", @progbits
.global text_cursor
text_cursor:
  LDA #4
  JMP bbc_vdu

.section ".text.graphics_text", "ax", @progbits
.global graphics_text
graphics_text:
  LDA #5
  JMP bbc_vdu

.section ".text.defchar", "ax", @progbits
.global defchar
defchar:
  LDY #2
  LDA (__sp), Y
  STA __t2
  INY
  LDA (__sp), Y
  STA __t2+1
  LDA #23
  JSR bbc_vdu
  LDY #0
  LDA (__sp), Y
  JSR bbc_vdu
  LDY #0
defchar_row:
  LDA (__t2), Y
  PHA
  TYA
  PHA
  TSX
  LDA 0x102, X
  JSR bbc_vdu
  PLA
  TAY
  PLA
  INY
  CPY #8
  BNE defchar_row
  RTS

.section ".text.vdu", "ax", @progbits
.global vdu
vdu:
  LDY #0
  LDA (__sp), Y
  JMP bbc_vdu

.section ".text.vdus", "ax", @progbits
.global vdus
vdus:
  LDY #0
  LDA (__sp), Y
  STA __t2
  INY
  LDA (__sp), Y
  STA __t2+1
  LDY #2
  LDA (__sp), Y
  STA bbc_vlen
  INY
  LDA (__sp), Y
  STA bbc_vlen+1
vdus_loop:
  LDA bbc_vlen
  ORA bbc_vlen+1
  BEQ vdus_done
  LDY #0
  LDA (__t2), Y
  JSR bbc_vdu
  INC __t2
  BNE vdus_next
  INC __t2+1
vdus_next:
  LDA bbc_vlen
  BNE vdus_dec
  DEC bbc_vlen+1
vdus_dec:
  DEC bbc_vlen
  JMP vdus_loop
vdus_done:
  RTS

.section ".text.beep", "ax", @progbits
.global beep
beep:
  LDA #7
  JMP bbc_vdu

.section ".text.paged", "ax", @progbits
.global paged
paged:
  LDY #0
  LDA (__sp), Y
  BEQ paged_off
  LDA #14
  JMP bbc_vdu
paged_off:
  LDA #15
  JMP bbc_vdu

.section ".text.colours_reset", "ax", @progbits
.global colours_reset
colours_reset:
  LDA #20
  JMP bbc_vdu

.section ".text.windows_reset", "ax", @progbits
.global windows_reset
windows_reset:
  LDA #26
  JMP bbc_vdu
