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
// Must hold a full-width mode-1 span: 320px / 4 = 80 bytes/row, plus header.
// A 128-byte buffer overflowed on deploy's 320x2 ground lines and corrupted
// the following blitter state — that returned to BASIC right after startup.
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

// ---------------------------------------------------------------------------
// C++ Screen::{close,fill,stamp} — hand-written to avoid the fat ABI helpers
// that inflate the compiled methods (~1.1 KiB → a few hundred bytes).
// Args are on the software stack at entry (caller-cleaned void).
// ---------------------------------------------------------------------------

.global _ZN6Screen4pen_E
_ZN6Screen4pen_E:
scr_pen:
.word batch_base

scr_paint:
.byte 0
scr_across:
.byte 0
scr_h:
.byte 0
scr_bytes:
.word 0
scr_dst:
.word 0
scr_src:
.word 0

.global _ZN6Screen5closeEv
_ZN6Screen5closeEv:
  LDA scr_pen
  CMP #%lo(batch_base)
  BNE sc_flush
  LDA scr_pen+1
  CMP #%hi(batch_base)
  BEQ sc_done
sc_flush:
  LDA __sp
  SEC
  SBC #2
  STA __sp
  LDA __sp+1
  SBC #0
  STA __sp+1
  LDY #0
  LDA scr_pen
  STA (__sp), Y
  INY
  LDA scr_pen+1
  STA (__sp), Y
  JSR batch_flush
  CLC
  LDA __sp
  ADC #2
  STA __sp
  LDA __sp+1
  ADC #0
  STA __sp+1
sc_done:
  LDA #%lo(batch_base)
  STA scr_pen
  LDA #%hi(batch_base)
  STA scr_pen+1
  RTS

// Ensure room for A = header(5) + payload size in scr_bytes.
scr_room:
  CLC
  LDA scr_pen
  ADC #5
  STA scr_dst
  LDA scr_pen+1
  ADC #0
  STA scr_dst+1
  CLC
  LDA scr_dst
  ADC scr_bytes
  STA scr_dst
  LDA scr_dst+1
  ADC scr_bytes+1
  STA scr_dst+1
  LDA scr_dst+1
  CMP #%hi(batch_limit)
  BCC scr_room_ok
  BNE scr_room_flush
  LDA scr_dst
  CMP #%lo(batch_limit)
  BCC scr_room_ok
  BEQ scr_room_ok
scr_room_flush:
  JSR _ZN6Screen5closeEv
  // After flush, re-check. If it still does not fit, clamp payload so we
  // never write past batch_limit (corrupt blit_* / code → BRK to BASIC).
  CLC
  LDA scr_pen
  ADC #5
  STA scr_dst
  LDA scr_pen+1
  ADC #0
  STA scr_dst+1
  CLC
  LDA scr_dst
  ADC scr_bytes
  STA __t0
  LDA scr_dst+1
  ADC scr_bytes+1
  STA __t1
  LDA __t1
  CMP #%hi(batch_limit)
  BCC scr_room_ok
  BNE scr_room_clamp
  LDA __t0
  CMP #%lo(batch_limit)
  BCC scr_room_ok
  BEQ scr_room_ok
scr_room_clamp:
  LDA #0
  STA scr_bytes
  STA scr_bytes+1
scr_room_ok:
  RTS

// Write 5-byte command header from stack args: x(0), y(2 low), across, h.
// Uses __i0 as temp pointer to header.
scr_header:
  LDA scr_pen
  STA __i0
  LDA scr_pen+1
  STA __i0+1
  LDY #0
  LDA (__sp), Y
  STA (__i0), Y
  INY
  LDA (__sp), Y
  STA (__i0), Y
  INY
  LDA (__sp), Y
  STA (__i0), Y
  INY
  LDA scr_across
  STA (__i0), Y
  INY
  LDA scr_h
  STA (__i0), Y
  CLC
  LDA scr_pen
  ADC #5
  STA scr_dst
  LDA scr_pen+1
  ADC #0
  STA scr_dst+1
  RTS

.global _ZN6Screen4fillEiiiii
// fill(x, y, w, h, colour) — ints at __sp + 0,2,4,6,8
_ZN6Screen4fillEiiiii:
  LDY #4
  LDA (__sp), Y
  INY
  ORA (__sp), Y
  BNE sf_w_ok
  RTS
sf_w_ok:
  LDY #6
  LDA (__sp), Y
  INY
  ORA (__sp), Y
  BNE sf_h_ok
  RTS
sf_h_ok:
  // Width is a 16-bit int. Using only the low byte made fill(..., 320, ...)
  // paint a 64px stub (320 & 0xff == 64) — the green strip on the left edge.
  LDY #4
  LDA (__sp), Y
  STA __t0
  INY
  LDA (__sp), Y
  LSR A
  ROR __t0
  LSR A
  ROR __t0
  LDA __t0
  STA scr_across
  LDY #6
  LDA (__sp), Y
  STA scr_h
  LDA #0
  STA scr_bytes+1
  STA scr_bytes
  LDX scr_h
sf_mul:
  BEQ sf_mul_done
  CLC
  LDA scr_bytes
  ADC scr_across
  STA scr_bytes
  LDA scr_bytes+1
  ADC #0
  STA scr_bytes+1
  DEX
  JMP sf_mul
sf_mul_done:
  LDY #8
  LDA (__sp), Y
  LDX #0
  CMP #1
  BNE sf_c2
  LDX #0x0f
  JMP sf_paint
sf_c2:
  CMP #2
  BNE sf_c3
  LDX #0xf0
  JMP sf_paint
sf_c3:
  CMP #3
  BNE sf_paint
  LDX #0xff
sf_paint:
  STX scr_paint
  JSR scr_room
  JSR scr_header
  LDA scr_dst
  STA __i0
  LDA scr_dst+1
  STA __i0+1
  LDA scr_bytes
  STA __i1
  LDA scr_bytes+1
  STA __i1+1
sf_loop:
  LDA __i1
  ORA __i1+1
  BEQ sf_done
  LDA scr_paint
  LDY #0
  STA (__i0), Y
  INC __i0
  BNE sf_dec
  INC __i0+1
sf_dec:
  LDA __i1
  BNE sf_dec1
  DEC __i1+1
sf_dec1:
  DEC __i1
  JMP sf_loop
sf_done:
  LDA __i0
  STA scr_pen
  LDA __i0+1
  STA scr_pen+1
  RTS

.global _ZN6Screen5stampEiiPKhii
// stamp(x, y, bits*, w, h) — ints/ptr at __sp + 0,2,4,6,8
_ZN6Screen5stampEiiPKhii:
  LDY #6
  LDA (__sp), Y
  INY
  ORA (__sp), Y
  BNE ss_w_ok
  RTS
ss_w_ok:
  LDY #8
  LDA (__sp), Y
  INY
  ORA (__sp), Y
  BNE ss_h_ok
  RTS
ss_h_ok:
  // Same 16-bit width handling as fill (pixels → mode-1 bytes).
  LDY #6
  LDA (__sp), Y
  STA __t0
  INY
  LDA (__sp), Y
  LSR A
  ROR __t0
  LSR A
  ROR __t0
  LDA __t0
  STA scr_across
  LDY #8
  LDA (__sp), Y
  STA scr_h
  LDA #0
  STA scr_bytes+1
  STA scr_bytes
  LDX scr_h
ss_mul:
  BEQ ss_mul_done
  CLC
  LDA scr_bytes
  ADC scr_across
  STA scr_bytes
  LDA scr_bytes+1
  ADC #0
  STA scr_bytes+1
  DEX
  JMP ss_mul
ss_mul_done:
  LDY #4
  LDA (__sp), Y
  STA scr_src
  INY
  LDA (__sp), Y
  STA scr_src+1
  JSR scr_room
  JSR scr_header
  LDA scr_dst
  STA __i0
  LDA scr_dst+1
  STA __i0+1
  LDA scr_src
  STA __i2
  LDA scr_src+1
  STA __i2+1
  LDA scr_bytes
  STA __i1
  LDA scr_bytes+1
  STA __i1+1
ss_loop:
  LDA __i1
  ORA __i1+1
  BEQ ss_done
  LDY #0
  LDA (__i2), Y
  STA (__i0), Y
  INC __i0
  BNE ss_a
  INC __i0+1
ss_a:
  INC __i2
  BNE ss_dec
  INC __i2+1
ss_dec:
  LDA __i1
  BNE ss_dec1
  DEC __i1+1
ss_dec1:
  DEC __i1
  JMP ss_loop
ss_done:
  LDA __i0
  STA scr_pen
  LDA __i0+1
  STA scr_pen+1
  RTS
