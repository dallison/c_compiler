// Snake play helpers for snake.cc: vsync, keyboard scan, HUD digits, and
// a one-cell paint. Lives in .text.blitter below &3000 with screen1.s so
// the shadow screen can stay mapped while fx_* runs. Units and key bits
// are in snake.h.

#include "snake.h"

.set OSBYTE 0xfff4
.set SYSVIA_LATCH 0xfe40
.set SYSVIA_DDRA 0xfe43
.set SYSVIA_PORTA 0xfe4f

.set E_T0 0x78
.set E_T1 0x79
.set E_C 0x7a
.set E_R 0x7b
.set E_IDX 0x7c

.section ".text.blitter", "ax", @progbits

// OSBYTE 19 straight — through bbc::osbyte it cost ~2K cycles a frame.
.global frame_wait
frame_wait:
  LDA #19
  JMP OSBYTE

// Score and best (to 9999) plus lives (to 9) in white under the labels.
// hud_places lives in snake.cc.
.global hud_digits
hud_digits:
  LDA score
  STA E_T0
  LDA score+1
  STA E_T1
  LDA #SN_HUD_SCORE_COL
  STA E_C
  JSR hud_u16
  LDA best
  STA E_T0
  LDA best+1
  STA E_T1
  LDA #SN_HUD_BEST_COL
  STA E_C
  JSR hud_u16
  LDA lives
  LDX #SN_HUD_LIVES_COL
  STX E_C
  CMP #10
  BCC hud_glyph
  LDA #9
  JMP hud_glyph

hud_u16:
  CMP #0x27
  BCC hu_ok
  BNE hu_clamp
  LDA E_T0
  CMP #0x10
  BCC hu_ok
hu_clamp:
  LDA #0x0f
  STA E_T0
  LDA #0x27
  STA E_T1
hu_ok:
  LDX #0
hu_place:
  LDY #0
hu_sub:
  SEC
  LDA E_T0
  SBC hud_places, X
  STA E_R
  LDA E_T1
  SBC hud_places+1, X
  BCC hu_digit
  STA E_T1
  LDA E_R
  STA E_T0
  INY
  BNE hu_sub
hu_digit:
  STX E_IDX
  TYA
  JSR hud_glyph
  LDX E_IDX
  INX
  INX
  CPX #8
  BNE hu_place
  RTS

// A = digit at byte column E_C; advances E_C by a text column.
hud_glyph:
  ASL A
  ASL A
  ASL A
  ASL A
  CLC
  ADC #%lo(k_digits)
  STA fx_src
  LDA #%hi(k_digits)
  ADC #0
  STA fx_src+1
  LDA E_C
  STA fx_col
  CLC
  ADC #2
  STA E_C
  LDA #SN_HUD_ROW
  STA fx_row
  LDA #16
  STA fx_len
  JMP fx_cell

// Scans the keys on the System VIA the MOS way for a single key. key_codes
// and key_masks live in snake.cc.
.global keys_read
keys_read:
  LDA #0
  STA key_bits
  SEI
  LDA #0x7f
  STA SYSVIA_DDRA
  LDA #0x03
  STA SYSVIA_LATCH
  LDX #KEY_COUNT-1
kr_loop:
  LDA key_codes, X
  STA SYSVIA_PORTA
  LDA SYSVIA_PORTA
  BPL kr_up
  LDA key_bits
  ORA key_masks, X
  STA key_bits
kr_up:
  DEX
  BPL kr_loop
  LDA #0x0b
  STA SYSVIA_LATCH
  CLI
  RTS

// Paint one board cell: sn_col / sn_row are board coords, sn_src the
// 16-byte 8x8 sprite. Used from C++ for head / body / fruit / rock.
.global cell_draw
cell_draw:
  LDA sn_col
  ASL A
  CLC
  ADC #SN_ORIGIN_COL
  STA fx_col
  LDA sn_row
  CLC
  ADC #SN_ORIGIN_ROW
  STA fx_row
  LDA sn_src
  STA fx_src
  LDA sn_src+1
  STA fx_src+1
  LDA #16
  STA fx_len
  JMP fx_cell

.global cell_wipe
cell_wipe:
  LDA sn_col
  ASL A
  CLC
  ADC #SN_ORIGIN_COL
  STA fx_col
  LDA sn_row
  CLC
  ADC #SN_ORIGIN_ROW
  STA fx_row
  LDA #16
  STA fx_len
  JMP fx_wipe
