#include "vars.s"

// Mode 1 paint for the Master shadow screen.
//
// Mode 1 is 320x256, four colours, 4 pixels per byte. With ACCCON bit X
// set, stores at &3000 reach the shadow picture while this code, which
// lives below &3000, keeps running. Bit D (display shadow) is left alone.
// Everything read while bit X is set (sources, tables, the fx_* variables)
// must also sit below &3000, and the MOS must not run, so interrupts are
// off for the length of each paint.
//
// Screen layout: character row r starts at &3000 + r*640. Byte column c
// (pixels 4c..4c+3) of that row is 8 bytes, one per scanline, at +c*8, so
// an 8-row cell is contiguous and so are the columns of one character row.
//
// fx_cell / fx_wipe work on cells: fx_len bytes from (fx_col, fx_row),
// column-major, at most 128 and within one character row.
// fx_strip / fx_blank / fx_peek walk fx_len scanlines (1..255) down byte
// column fx_col from pixel row fx_y.

.set FX_DST 0x70
.set FX_SRC 0x72
.set FX_T0 0x74
.set FX_T1 0x75
.set FX_T2 0x76
.set FX_T3 0x77

.section ".text.blitter", "ax", @progbits

.global fx_col
fx_col:
.byte 0
.global fx_row
fx_row:
.byte 0
.global fx_y
fx_y:
.byte 0
.global fx_len
fx_len:
.byte 0
.global fx_src
fx_src:
.hword 0

row_lo:
.byte 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80
.byte 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80
.byte 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80
.byte 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80
row_hi:
.byte 0x30, 0x32, 0x35, 0x37, 0x3a, 0x3c, 0x3f, 0x41
.byte 0x44, 0x46, 0x49, 0x4b, 0x4e, 0x50, 0x53, 0x55
.byte 0x58, 0x5a, 0x5d, 0x5f, 0x62, 0x64, 0x67, 0x69
.byte 0x6c, 0x6e, 0x71, 0x73, 0x76, 0x78, 0x7b, 0x7d

paint_tab:
.byte 0x00, 0x0f, 0xf0, 0xff

.global note_acccon
note_acccon:
  LDA 0xfe34
  STA acccon_copy
  RTS

.global shadow_on
shadow_on:
  SEI
  LDA 0xfe34
  ORA #0x04
  STA 0xfe34
  RTS

.global shadow_off
shadow_off:
  LDA 0xfe34
  AND #0xfb
  STA 0xfe34
  CLI
  RTS

// FX_DST = start of byte column A (0..79) on character row X (0..31).
cell_addr:
  LDY #0
  STY FX_T0
  ASL A
  ROL FX_T0
  ASL A
  ROL FX_T0
  ASL A
  ROL FX_T0
  CLC
  ADC row_lo, X
  STA FX_DST
  LDA FX_T0
  ADC row_hi, X
  STA FX_DST+1
  RTS

// FX_DST = cell of (fx_col, fx_y), Y = scanline within it.
strip_addr:
  LDA fx_y
  LSR A
  LSR A
  LSR A
  TAX
  LDA fx_col
  JSR cell_addr
  LDA fx_y
  AND #7
  TAY
  RTS

// Below scanline 7 a byte column continues at the top of the next
// character row, 640 bytes on.
next_char_row:
  LDY #0
  CLC
  LDA FX_DST
  ADC #0x80
  STA FX_DST
  LDA FX_DST+1
  ADC #0x02
  STA FX_DST+1
  RTS

.global fx_cell
fx_cell:
  LDA fx_src
  STA FX_SRC
  LDA fx_src+1
  STA FX_SRC+1
  LDX fx_row
  LDA fx_col
  JSR cell_addr
  JSR shadow_on
  LDY fx_len
  DEY
fc_loop:
  LDA (FX_SRC), Y
  STA (FX_DST), Y
  DEY
  BPL fc_loop
  JMP shadow_off

.global fx_wipe
fx_wipe:
  LDX fx_row
  LDA fx_col
  JSR cell_addr
  JSR shadow_on
  LDY fx_len
  DEY
  LDA #0
fw_loop:
  STA (FX_DST), Y
  DEY
  BPL fw_loop
  JMP shadow_off

.global fx_strip
fx_strip:
  LDA fx_src
  STA fs_src+1
  LDA fx_src+1
  STA fs_src+2
  JSR strip_addr
  JSR shadow_on
  LDX #0
fs_loop:
fs_src:
  LDA 0xffff, X
  STA (FX_DST), Y
  INX
  CPX fx_len
  BEQ fs_done
  INY
  CPY #8
  BNE fs_loop
  JSR next_char_row
  JMP fs_loop
fs_done:
  JMP shadow_off

.global fx_blank
fx_blank:
  JSR strip_addr
  JSR shadow_on
  LDX fx_len
fb_loop:
  LDA #0
  STA (FX_DST), Y
  DEX
  BEQ fb_done
  INY
  CPY #8
  BNE fb_loop
  JSR next_char_row
  JMP fb_loop
fb_done:
  JMP shadow_off

// A = the OR of the bytes under the strip: nonzero when anything is lit.
.global fx_peek
fx_peek:
  JSR strip_addr
  JSR shadow_on
  LDX fx_len
  LDA #0
  STA FX_T1
fp_loop:
  LDA (FX_DST), Y
  ORA FX_T1
  STA FX_T1
  DEX
  BEQ fp_done
  INY
  CPY #8
  BNE fp_loop
  JSR next_char_row
  JMP fp_loop
fp_done:
  JSR shadow_off
  LDA FX_T1
  RTS

// ---------------------------------------------------------------------------
// C++ Screen::fill(x, y, w, h, colour): ints at __sp + 0,2,4,6,8, caller
// cleaned. x and w are pixels and multiples of 4; y and h are any rows.
// ---------------------------------------------------------------------------

sf_across:
.byte 0
sf_rows:
.byte 0
sf_ink:
.byte 0

sf_out:
  RTS

.global _ZN6Screen4fillEiiiii
_ZN6Screen4fillEiiiii:
  LDY #4
  LDA (__sp), Y
  INY
  ORA (__sp), Y
  BEQ sf_out
  LDY #6
  LDA (__sp), Y
  BEQ sf_out
  STA sf_rows
  // Width is a 16-bit int: 320 & 0xff alone would paint a 64px stub.
  LDY #4
  LDA (__sp), Y
  STA FX_T0
  INY
  LDA (__sp), Y
  LSR A
  ROR FX_T0
  LSR A
  ROR FX_T0
  LDA FX_T0
  BEQ sf_out
  STA sf_across
  LDY #8
  LDA (__sp), Y
  AND #3
  TAX
  LDA paint_tab, X
  STA sf_ink
  LDY #2
  LDA (__sp), Y
  STA fx_y
  LDY #1
  LDA (__sp), Y
  LSR A
  DEY
  LDA (__sp), Y
  ROR A
  LSR A
  STA fx_col
  JSR strip_addr
  STY FX_T2
  JSR shadow_on
sf_row:
  CLC
  LDA FX_DST
  ADC FX_T2
  STA FX_SRC
  LDA FX_DST+1
  ADC #0
  STA FX_SRC+1
  LDX sf_across
  LDY #0
sf_byte:
  LDA sf_ink
  STA (FX_SRC), Y
  CLC
  LDA FX_SRC
  ADC #8
  STA FX_SRC
  BCC sf_nc
  INC FX_SRC+1
sf_nc:
  DEX
  BNE sf_byte
  DEC sf_rows
  BEQ sf_done
  INC FX_T2
  LDA FX_T2
  CMP #8
  BNE sf_row
  JSR next_char_row
  STY FX_T2
  JMP sf_row
sf_done:
  JMP shadow_off
