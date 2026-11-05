// Asteroids play helpers for asteroids.cc: vsync, keyboard scan, HUD
// digits, and cell paints. Lives in .text.blitter below &3000 with
// screen1.s. Units and key bits are in asteroids.h.

#include "asteroids.h"

.set OSBYTE 0xfff4
.set SYSVIA_LATCH 0xfe40
.set SYSVIA_DDRA 0xfe43
.set SYSVIA_PORTA 0xfe4f

// Keep soft-blit temps out of &70-&7F (OS/fp scratch). davecc ends at &5C.
.set E_T0 0x60
.set E_T1 0x61
.set E_C 0x62
.set E_R 0x63
.set E_IDX 0x64

.section ".text.blitter", "ax", @progbits

.global frame_wait
frame_wait:
  LDA #19
  JMP OSBYTE

.global hud_digits
hud_digits:
  LDA score
  STA E_T0
  LDA score+1
  STA E_T1
  LDA #AS_HUD_SCORE_COL
  STA E_C
  JSR hud_u16
  LDA best
  STA E_T0
  LDA best+1
  STA E_T1
  LDA #AS_HUD_BEST_COL
  STA E_C
  JSR hud_u16
  LDA lives
  LDX #AS_HUD_LIVES_COL
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
  LDA #AS_HUD_ROW
  STA fx_row
  LDA #16
  STA fx_len
  JMP fx_cell

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

// Paint an 8x8 sprite: as_col / as_row are byte column and character row,
// as_src the 16-byte bitmap.
.global cell_draw
cell_draw:
  LDA as_col
  STA fx_col
  LDA as_row
  STA fx_row
  LDA as_src
  STA fx_src
  LDA as_src+1
  STA fx_src+1
  LDA #AS_CELL_LEN
  STA fx_len
  JMP fx_cell

.global cell_wipe
cell_wipe:
  LDA as_col
  STA fx_col
  LDA as_row
  STA fx_row
  LDA #AS_CELL_LEN
  STA fx_len
  JMP fx_wipe

// Large rock: 16x16 as two 32-byte character-row windows. as_src points at
// the top half; the bottom half follows immediately.
.global cell_draw_large
cell_draw_large:
  LDA as_col
  STA fx_col
  LDA as_row
  STA fx_row
  LDA as_src
  STA fx_src
  LDA as_src+1
  STA fx_src+1
  LDA #AS_LARGE_LEN
  STA fx_len
  JSR fx_cell
  LDA as_col
  STA fx_col
  LDA as_row
  CLC
  ADC #1
  STA fx_row
  CLC
  LDA as_src
  ADC #AS_LARGE_LEN
  STA fx_src
  LDA as_src+1
  ADC #0
  STA fx_src+1
  LDA #AS_LARGE_LEN
  STA fx_len
  JMP fx_cell

.global cell_wipe_large
cell_wipe_large:
  LDA as_col
  STA fx_col
  LDA as_row
  STA fx_row
  LDA #AS_LARGE_LEN
  STA fx_len
  JSR fx_wipe
  LDA as_col
  STA fx_col
  LDA as_row
  CLC
  ADC #1
  STA fx_row
  LDA #AS_LARGE_LEN
  STA fx_len
  JMP fx_wipe

// Missile strip: as_col = byte column, as_y = top pixel row, as_src =
// AS_SHOT_LEN bytes (one Mode-1 byte per row).
.global shot_draw
shot_draw:
  LDA as_col
  STA fx_col
  LDA as_y
  STA fx_y
  LDA as_src
  STA fx_src
  LDA as_src+1
  STA fx_src+1
  LDA #AS_SHOT_LEN
  STA fx_len
  JMP fx_strip

.global shot_wipe
shot_wipe:
  LDA as_col
  STA fx_col
  LDA as_y
  STA fx_y
  LDA #AS_SHOT_LEN
  STA fx_len
  JMP fx_blank

// Soft rock: as_col / as_y / as_src. Column-major strips so Y is a pixel
// row — the old character-cell blit was why rocks stepped in jerks.
.global soft_draw_large
soft_draw_large:
  LDA #AS_SOFT_LARGE_COLS
  STA E_T1
  LDA #AS_SOFT_LARGE_ROWS
  JMP soft_draw

.global soft_wipe_large
soft_wipe_large:
  LDA #AS_SOFT_LARGE_COLS
  STA E_T1
  LDA #AS_SOFT_LARGE_ROWS
  JMP soft_wipe

.global soft_draw_small
soft_draw_small:
  LDA #AS_SOFT_SMALL_COLS
  STA E_T1
  LDA #AS_SOFT_SMALL_ROWS
  JMP soft_draw

.global soft_wipe_small
soft_wipe_small:
  LDA #AS_SOFT_SMALL_COLS
  STA E_T1
  LDA #AS_SOFT_SMALL_ROWS
  JMP soft_wipe

// Ship cell16 at pixel Y: each of 4 columns is top 8 + bottom 8 from
// the character-row halves (no bake buffer — keeps the DFS image under HIMEM).
.global soft_draw_ship
soft_draw_ship:
  LDA #0
  STA E_T0
sds_loop:
  LDA as_col
  CLC
  ADC E_T0
  STA fx_col
  LDA as_y
  STA fx_y
  LDA E_T0
  ASL A
  ASL A
  ASL A
  CLC
  ADC as_src
  STA fx_src
  LDA as_src+1
  ADC #0
  STA fx_src+1
  LDA #8
  STA fx_len
  JSR fx_strip
  LDA as_y
  CLC
  ADC #8
  BCS sds_next
  STA fx_y
  LDA as_col
  CLC
  ADC E_T0
  STA fx_col
  LDA E_T0
  ASL A
  ASL A
  ASL A
  CLC
  ADC #AS_LARGE_LEN
  ADC as_src
  STA fx_src
  LDA as_src+1
  ADC #0
  STA fx_src+1
  LDA #8
  STA fx_len
  JSR fx_strip
sds_next:
  INC E_T0
  LDA E_T0
  CMP #AS_SOFT_SHIP_COLS
  BCC sds_loop
  RTS

.global soft_wipe_ship
soft_wipe_ship:
  LDA #AS_SOFT_SHIP_COLS
  STA E_T1
  LDA #AS_SOFT_SHIP_ROWS
  JMP soft_wipe

// Clamp rows so as_y + rows does not wrap past pixel 255 — strip_addr
// only has 32 character-row entries; past that it writes garbage and
// shreds the soft stack / returns to BASIC with a trashed screen.
soft_clamp_rows:
  STA E_R
  LDA as_y
  CLC
  ADC E_R
  BCC scr_ok
  LDA #0
  SEC
  SBC as_y
  STA E_R
scr_ok:
  LDA E_R
  RTS

soft_draw:
  JSR soft_clamp_rows
  BEQ sd_out
  LDA #0
  STA E_T0
sd_loop:
  LDA as_col
  CLC
  ADC E_T0
  STA fx_col
  LDA as_y
  STA fx_y
  LDA E_R
  CMP #16
  BEQ sd_x16
  LDA E_T0
  ASL A
  ASL A
  ASL A
  JMP sd_src
sd_x16:
  LDA E_T0
  ASL A
  ASL A
  ASL A
  ASL A
sd_src:
  CLC
  ADC as_src
  STA fx_src
  LDA as_src+1
  ADC #0
  STA fx_src+1
  LDA E_R
  STA fx_len
  JSR fx_strip
  INC E_T0
  LDA E_T0
  CMP E_T1
  BCC sd_loop
sd_out:
  RTS

soft_wipe:
  JSR soft_clamp_rows
  BEQ sw_out
  LDA #0
  STA E_T0
sw_loop:
  LDA as_col
  CLC
  ADC E_T0
  STA fx_col
  LDA as_y
  STA fx_y
  LDA E_R
  STA fx_len
  JSR fx_blank
  INC E_T0
  LDA E_T0
  CMP E_T1
  BCC sw_loop
sw_out:
  RTS
