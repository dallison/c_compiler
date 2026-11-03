#include "vars.s"

// Mode 1 paint for the Master shadow screen.
//
// Mode 1 is 320x256, four colours, 4 pixels per byte. With ACCCON bit X
// set, stores at &3000 reach the shadow picture while this code, which
// lives below &3000, keeps running. Bit D (display shadow) is left alone.
//
// A command is 5 bytes then the pixels: x low, x high, y, bytes per row,
// height, then (bytes * height) mode-1 bytes. batch_flush walks from
// batch_base up to the pointer it is given, in one shadow session.

.set SRC 0x8c
.set DST 0x8e

.section ".text.blitter", "ax", @progbits

.global batch_base
.global batch_limit
batch_base:
.space 256
batch_limit:

blit_x:
.word 0
blit_y:
.byte 0
blit_wb:
.byte 0
blit_h:
.byte 0
blit_ptr:
.word 0
blit_s0:
.word 0
blit_s1:
.word 0
stream:
.word 0
stream_end:
.word 0
cmd_count:
.byte 0

.global note_acccon
note_acccon:
  LDA 0xfe34
  STA acccon_copy
  RTS

.global batch_flush
batch_flush:
  LDA 0x8c
  PHA
  LDA 0x8d
  PHA
  LDA 0x8e
  PHA
  LDA 0x8f
  PHA
  LDY #0
  LDA (__sp), Y
  STA stream_end
  INY
  LDA (__sp), Y
  STA stream_end+1
  LDA #%lo(batch_base)
  STA stream
  LDA #%hi(batch_base)
  STA stream+1
  JSR shadow_on
flush_cmd:
  LDA stream+1
  CMP stream_end+1
  BCC flush_go
  BNE flush_done
  LDA stream
  CMP stream_end
  BCS flush_done
flush_go:
  LDA stream
  STA SRC
  LDA stream+1
  STA SRC+1
  LDY #0
  LDA (SRC), Y
  STA blit_x
  INY
  LDA (SRC), Y
  STA blit_x+1
  INY
  LDA (SRC), Y
  STA blit_y
  INY
  LDA (SRC), Y
  STA blit_wb
  INY
  LDA (SRC), Y
  STA blit_h
  JSR stream_add5
  LDA blit_h
  BEQ flush_cmd
  LDA blit_wb
  BEQ flush_cmd
  JSR paint_rows
  JMP flush_cmd
flush_done:
  JSR shadow_off
  PLA
  STA 0x8f
  PLA
  STA 0x8e
  PLA
  STA 0x8d
  PLA
  STA 0x8c
  RTS

// Advance the stream by 5. Shadow may be on; this uses only low memory.
stream_add5:
  CLC
  LDA stream
  ADC #5
  STA stream
  BCC sa5
  INC stream+1
sa5:
  RTS

// Paint blit_h rows of blit_wb bytes from the stream. Each row is contiguous
// in the stream. On screen, the next byte in a row is 8 bytes away.
paint_rows:
  LDA stream
  STA SRC
  LDA stream+1
  STA SRC+1
  LDA #0
  STA cmd_count
pr_row:
  LDA blit_h
  BEQ pr_done
  JSR make_addr
  LDA #0
  STA blit_s0
pr_col:
  LDA blit_s0
  CMP blit_wb
  BEQ pr_next
  LDY cmd_count
  LDA (SRC), Y
  JSR poke
  INC cmd_count
  INC blit_s0
  JMP pr_col
pr_next:
  INC blit_y
  DEC blit_h
  JMP pr_row
pr_done:
  CLC
  LDA stream
  ADC cmd_count
  STA stream
  LDA stream+1
  ADC #0
  STA stream+1
  RTS

// Write A to blit_ptr and step to the next byte of this scanline.
poke:
  PHA
  LDA blit_ptr
  STA DST
  LDA blit_ptr+1
  STA DST+1
  LDY #0
  PLA
  STA (DST), Y
  CLC
  LDA blit_ptr
  ADC #8
  STA blit_ptr
  BCC poke_nc
  INC blit_ptr+1
poke_nc:
  RTS

// blit_ptr = &3000 + (y >> 3) * 640 + (y & 7) + (x >> 2) * 8
make_addr:
  LDA blit_x
  STA blit_s0
  LDA blit_x+1
  STA blit_s0+1
  LSR blit_s0+1
  ROR blit_s0
  LSR blit_s0+1
  ROR blit_s0
  ASL blit_s0
  ROL blit_s0+1
  ASL blit_s0
  ROL blit_s0+1
  ASL blit_s0
  ROL blit_s0+1
  LDA blit_y
  LSR A
  LSR A
  LSR A
  STA blit_s1
  LDA #0
  STA blit_s1+1
  LDX #7
ma_sh:
  ASL blit_s1
  ROL blit_s1+1
  DEX
  BNE ma_sh
  LDA blit_s1
  STA blit_ptr
  LDA blit_s1+1
  STA blit_ptr+1
  ASL blit_s1
  ROL blit_s1+1
  ASL blit_s1
  ROL blit_s1+1
  CLC
  LDA blit_s1
  ADC blit_ptr
  STA blit_s1
  LDA blit_s1+1
  ADC blit_ptr+1
  STA blit_s1+1
  CLC
  LDA blit_s1
  ADC #0x00
  STA blit_ptr
  LDA blit_s1+1
  ADC #0x30
  STA blit_ptr+1
  LDA blit_y
  AND #7
  CLC
  ADC blit_ptr
  STA blit_ptr
  BCC ma_sc
  INC blit_ptr+1
ma_sc:
  CLC
  LDA blit_ptr
  ADC blit_s0
  STA blit_ptr
  LDA blit_ptr+1
  ADC blit_s0+1
  STA blit_ptr+1
  RTS

shadow_on:
  SEI
  LDA 0xfe34
  ORA #0x04
  STA 0xfe34
  RTS

shadow_off:
  LDA 0xfe34
  AND #0xfb
  STA 0xfe34
  CLI
  RTS
