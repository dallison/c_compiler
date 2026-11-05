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
// rocks_step temps (0x65-0x6e; screen1.s owns 0x70-0x77, davecc ZP ends
// at 0x5c). Nothing here is live across a paint call except R_IDX/R_OFF.
.set R_IDX 0x65
.set R_OFF 0x66
.set R_COL 0x67
.set R_SY 0x68
.set R_PH 0x69
.set R_T0 0x6a
.set R_RAD 0x6c
.set R_DX 0x6d
// rock_blit temps. B_BASE/B_CNT/B_SCAN overlap the soft_* temps
// (0x60-0x64) on purpose: the blit never runs inside those routines.
.set B_BASE 0x5d
.set B_CNT 0x5f
.set B_SCAN 0x60
.set B_NCOL 0x61
.set B_ROWS 0x62
.set B_COLS 0x63
.set B_TMPY 0x64
// B_OLDX shares R_RAD's byte: the wipe consumes it before the ship
// test sets R_RAD. (It must not share R_T0 — the image-index sum in
// rocks_step overwrites that between rs_wipe and the blit.)
.set B_OLDX 0x6c
.set B_OLDY 0x6b
.set B_WIPE 0x6f

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

.global soft_wipe_ship
soft_wipe_ship:
  LDA #AS_SOFT_SHIP_COLS
  STA E_T1
  LDA #AS_SOFT_SHIP_ROWS
  JMP soft_wipe

// ---------------------------------------------------------------- rocks

// Set by rocks_step when a rock reaches the ship; beat_rocks kills it.
.global as_hit
as_hit:
.byte 0

// Rocks repaint on alternate calls, staggered by index (see
// rs_moved): their wipe+draw is the dearest paint in the game.
rs_parity:
.byte 0

// Both phases of every rock look, in rock_image order
// (size * 4 + look * 2 + phase).
rock_img_tab:
  .hword k_as_rock_l0
  .hword k_as_rock_l0 + AS_SOFT_LARGE_PHASE
  .hword k_as_rock_l1
  .hword k_as_rock_l1 + AS_SOFT_LARGE_PHASE
  .hword k_as_rock_m0
  .hword k_as_rock_m0 + AS_SOFT_SMALL_PHASE
  .hword k_as_rock_m1
  .hword k_as_rock_m1 + AS_SOFT_SMALL_PHASE
  .hword k_as_rock_s0
  .hword k_as_rock_s0 + AS_SOFT_SMALL_PHASE
  .hword k_as_rock_s1
  .hword k_as_rock_s1 + AS_SOFT_SMALL_PHASE

rock_rad_tab:
.byte 10, 6, 3

// Advance, wrap, paint and ship-test every live rock. The field is
// x 0..303, y 16..239 (asteroids.cc's FieldW / FieldY0 / FieldY1).
// Doing this in C++ cost ~40K cycles a tick in call and register-save
// overhead alone and held the game at several vsyncs a tick, so — like
// the invaders fleet — the per-frame step lives here and C++ keeps the
// rules. A rock whose byte column, row and 2 px phase are unchanged is
// not repainted.
.global rocks_step
rocks_step:
  LDA #0
  STA as_hit
  STA R_IDX
  LDA rs_parity
  EOR #1
  STA rs_parity
rs_loop:
  LDX R_IDX
  CPX #AS_ROCKS
  BCC rs_body
  RTS
rs_body:
  LDA rock_on, X
  BNE rs_live
  JMP rs_next
rs_live:
  TXA
  ASL A
  STA R_OFF
  // Every rock moves every call; repaints are staggered by index
  // parity further down (see rs_moved).
rs_move:
  // x += vx (sign-extended), wrapped into 0..303.
  LDY R_OFF
  LDA rock_x, Y
  CLC
  ADC rock_vx, X
  STA rock_x, Y
  LDA rock_vx, X
  BMI rs_xneg
  LDA rock_x+1, Y
  ADC #0
  STA rock_x+1, Y
  JMP rs_xwrap
rs_xneg:
  LDA rock_x+1, Y
  ADC #0xff
  STA rock_x+1, Y
rs_xwrap:
  LDA rock_x+1, Y
  BPL rs_xchk
  LDA rock_x, Y
  CLC
  ADC #0x30
  STA rock_x, Y
  LDA rock_x+1, Y
  ADC #1
  STA rock_x+1, Y
  JMP rs_xdone
rs_xchk:
  LDA rock_x+1, Y
  CMP #2
  BCS rs_xsub
  CMP #1
  BNE rs_xdone
  LDA rock_x, Y
  CMP #0x30
  BCC rs_xdone
rs_xsub:
  LDA rock_x, Y
  SEC
  SBC #0x30
  STA rock_x, Y
  LDA rock_x+1, Y
  SBC #1
  STA rock_x+1, Y
rs_xdone:
  // y += vy (sign-extended), wrapped into 16..239 (224 tall).
  LDY R_OFF
  LDX R_IDX
  LDA rock_y, Y
  CLC
  ADC rock_vy, X
  STA rock_y, Y
  LDA rock_vy, X
  BMI rs_yneg
  LDA rock_y+1, Y
  ADC #0
  STA rock_y+1, Y
  JMP rs_ywrap
rs_yneg:
  LDA rock_y+1, Y
  ADC #0xff
  STA rock_y+1, Y
rs_ywrap:
  LDY R_OFF
  LDA rock_y, Y
  CMP #240
  BCS rs_ysub
  CMP #16
  BCS rs_ydone
  CLC
  ADC #224
  STA rock_y, Y
  JMP rs_ydone
rs_ysub:
  SEC
  SBC #224
  STA rock_y, Y
rs_ydone:
  // Byte column (x >> 2, minus the sprite's blank left column), pixel
  // row, and 2 px phase (bit 1 of x).
  LDY R_OFF
  LDA rock_x, Y
  LSR A
  LSR A
  STA R_COL
  LDA rock_x+1, Y
  AND #1
  BEQ rs_col0
  LDA R_COL
  CLC
  ADC #64
  STA R_COL
rs_col0:
  LDA R_COL
  BEQ rs_col1
  DEC R_COL
rs_col1:
  LDA rock_y, Y
  STA R_SY
  LDA rock_x, Y
  AND #2
  LSR A
  STA R_PH
  LDX R_IDX
  LDA rock_shown, X
  BEQ rs_noshow
  LDA R_COL
  CMP rock_col, X
  BNE rs_moved
  LDA R_SY
  CMP rock_sy, X
  BNE rs_moved
  LDA R_PH
  CMP rock_phase, X
  BNE rs_moved
  JMP rs_coll
rs_moved:
  // Repaint at half rate, staggered by index: the logical position
  // (and the ship/shot tests) advance every tick, but the wipe+draw
  // is the dearest work in the game, so each rock repaints on
  // alternate ticks. rock_col/rock_sy keep describing the drawn
  // window until then, so the eventual wipe still erases it exactly.
  LDA R_IDX
  AND #1
  CMP rs_parity
  BEQ rs_wipe
  JMP rs_coll
rs_wipe:
  LDA rock_col, X
  STA B_OLDX
  LDA rock_sy, X
  STA B_OLDY
  LDA #1
  STA B_WIPE
  JMP rs_draw
rs_noshow:
  LDA #0
  STA B_WIPE
rs_draw:
  LDX R_IDX
  LDA R_COL
  STA rock_col, X
  STA as_col
  LDA R_SY
  STA rock_sy, X
  STA as_y
  LDA R_PH
  STA rock_phase, X
  LDA rock_size, X
  ASL A
  ASL A
  STA R_T0
  LDA rock_look, X
  ASL A
  CLC
  ADC R_T0
  ADC R_PH
  ASL A
  TAY
  LDA rock_img_tab, Y
  STA as_src
  LDA rock_img_tab+1, Y
  STA as_src+1
  LDX R_IDX
  LDA rock_size, X
  BNE rs_blit_small
  LDA #AS_SOFT_LARGE_COLS
  STA B_COLS
  LDA #AS_SOFT_LARGE_ROWS
  STA B_ROWS
  JMP rs_blit
rs_blit_small:
  LDA #AS_SOFT_SMALL_COLS
  STA B_COLS
  LDA #AS_SOFT_SMALL_ROWS
  STA B_ROWS
rs_blit:
  JSR rock_blit
rs_shown:
  LDX R_IDX
  LDA #1
  STA rock_shown, X
rs_coll:
  LDA ship_on
  BNE rs_coll2
  JMP rs_next
rs_coll2:
  LDA ship_invuln
  BEQ rs_coll3
  JMP rs_next
rs_coll3:
  LDX R_IDX
  LDA rock_size, X
  TAY
  LDA rock_rad_tab, Y
  STA R_RAD
  // dx = |(ship_x + 8) - (rock_x + rad)|, as (ship_x - rock_x) + (8 - rad).
  LDY R_OFF
  LDA ship_x
  SEC
  SBC rock_x, Y
  STA R_DX
  LDA ship_x+1
  SBC rock_x+1, Y
  STA R_DX+1
  LDA #8
  SEC
  SBC R_RAD
  STA R_T0
  LDA R_DX
  CLC
  ADC R_T0
  STA R_DX
  LDA R_T0
  BMI rs_dxneg
  LDA R_DX+1
  ADC #0
  STA R_DX+1
  JMP rs_dxabs
rs_dxneg:
  LDA R_DX+1
  ADC #0xff
  STA R_DX+1
rs_dxabs:
  LDA R_DX+1
  BPL rs_dxchk
  LDA #0
  SEC
  SBC R_DX
  STA R_DX
  LDA #0
  SBC R_DX+1
  STA R_DX+1
rs_dxchk:
  LDA R_DX+1
  BNE rs_nohit
  LDA #5
  CLC
  ADC R_RAD
  CMP R_DX
  BCC rs_nohit
  BEQ rs_nohit
  // dy = |(ship_y + 8) - (rock_y + rad)|, the same way.
  LDY R_OFF
  LDA ship_y
  SEC
  SBC rock_y, Y
  STA R_DX
  LDA ship_y+1
  SBC rock_y+1, Y
  STA R_DX+1
  LDA #8
  SEC
  SBC R_RAD
  STA R_T0
  LDA R_DX
  CLC
  ADC R_T0
  STA R_DX
  LDA R_T0
  BMI rs_dyneg
  LDA R_DX+1
  ADC #0
  STA R_DX+1
  JMP rs_dyabs
rs_dyneg:
  LDA R_DX+1
  ADC #0xff
  STA R_DX+1
rs_dyabs:
  LDA R_DX+1
  BPL rs_dychk
  LDA #0
  SEC
  SBC R_DX
  STA R_DX
  LDA #0
  SBC R_DX+1
  STA R_DX+1
rs_dychk:
  LDA R_DX+1
  BNE rs_nohit
  LDA #5
  CLC
  ADC R_RAD
  CMP R_DX
  BCC rs_nohit
  BEQ rs_nohit
  LDA #1
  STA as_hit
  RTS
rs_nohit:
  JMP rs_next
rs_next:
  INC R_IDX
  JMP rs_loop

// ---------------------------------------------------------------- shots

// Set by shots_step: bit i = shot i died this call (expired or hit);
// as_shot_victim[i] = rock it hit, or 0xff. beat_shots does the
// erase/split bookkeeping (rare, and split_rock is big C++).
.global as_shot_dead
as_shot_dead:
.byte 0
.global as_shot_victim
as_shot_victim:
.byte 0xff, 0xff, 0xff, 0xff

ss_bits:
.byte 1, 2, 4, 8

shot_img_tab:
  .hword k_as_shot0
  .hword k_as_shot1
  .hword k_as_shot2
  .hword k_as_shot3

// Move every live shot, wrap it, and box-test it against every rock.
// In assembly for the same reason as rocks_step: the C++ loop indexed
// four shot arrays and twelve rock arrays per pair, and the helper
// calls (advance_x/advance_y/hits) cost more than the arithmetic.
// Temps reuse the rocks_step bytes; the two never run nested.
.global shots_step
shots_step:
  LDA #0
  STA as_shot_dead
  LDA #0xff
  STA as_shot_victim
  STA as_shot_victim+1
  STA as_shot_victim+2
  STA as_shot_victim+3
  LDA #0
  STA R_IDX
ss_loop:
  LDY R_IDX
  LDA shot_on, Y
  BNE ss_live
  JMP ss_next
ss_live:
  LDA shot_life, Y
  BNE ss_life1
  JMP ss_expire
ss_life1:
  SEC
  SBC #1
  STA shot_life, Y
  BNE ss_alive
  JMP ss_expire
ss_alive:
  LDA R_IDX
  ASL A
  STA R_OFF
  // x += vx (sign-extended), wrapped into 0..303.
  LDY R_OFF
  LDX R_IDX
  LDA shot_x, Y
  CLC
  ADC shot_vx, X
  STA shot_x, Y
  LDA shot_vx, X
  BMI ss_xneg
  LDA shot_x+1, Y
  ADC #0
  STA shot_x+1, Y
  JMP ss_xwrap
ss_xneg:
  LDA shot_x+1, Y
  ADC #0xff
  STA shot_x+1, Y
ss_xwrap:
  LDY R_OFF
  LDA shot_x+1, Y
  BMI ss_xadd
  CMP #2
  BCS ss_xsub
  CMP #1
  BNE ss_xdone
  LDA shot_x, Y
  CMP #0x30
  BCC ss_xdone
ss_xsub:
  LDY R_OFF
  LDA shot_x, Y
  SEC
  SBC #0x30
  STA shot_x, Y
  LDA shot_x+1, Y
  SBC #1
  STA shot_x+1, Y
  JMP ss_xdone
ss_xadd:
  LDY R_OFF
  LDA shot_x, Y
  CLC
  ADC #0x30
  STA shot_x, Y
  LDA shot_x+1, Y
  ADC #1
  STA shot_x+1, Y
ss_xdone:
  // y += vy, wrapped into 16..239 (224 tall), as in rocks_step.
  LDY R_OFF
  LDX R_IDX
  LDA shot_y, Y
  CLC
  ADC shot_vy, X
  STA shot_y, Y
  LDA shot_vy, X
  BMI ss_yneg
  LDA shot_y+1, Y
  ADC #0
  STA shot_y+1, Y
  JMP ss_ywrap
ss_yneg:
  LDA shot_y+1, Y
  ADC #0xff
  STA shot_y+1, Y
ss_ywrap:
  LDY R_OFF
  LDA shot_y, Y
  CMP #240
  BCS ss_ysub
  CMP #16
  BCS ss_ydone
  CLC
  ADC #224
  STA shot_y, Y
  JMP ss_ydone
ss_ysub:
  SEC
  SBC #224
  STA shot_y, Y
ss_ydone:
  // Box test against every rock: |dx| and |dy| < 2 + radius.
  LDX #0
ss_rloop:
  LDA rock_on, X
  BNE ss_ron
  JMP ss_rnext
ss_ron:
  TXA
  ASL A
  STA R_T0
  LDA rock_size, X
  STA B_CNT
  LDY B_CNT
  LDA rock_rad_tab, Y
  STA R_RAD
  // dx = (shot_x + 4) - (rock_x + rad), absolute.
  LDY R_T0
  LDA rock_x, Y
  CLC
  ADC R_RAD
  STA R_DX
  LDA rock_x+1, Y
  ADC #0
  STA R_DX+1
  LDY R_OFF
  LDA shot_x, Y
  CLC
  ADC #4
  SEC
  SBC R_DX
  STA R_DX
  LDA shot_x+1, Y
  ADC #0
  SBC R_DX+1
  STA R_DX+1
  LDA R_DX+1
  BPL ss_dxchk
  LDA #0
  SEC
  SBC R_DX
  STA R_DX
  LDA #0
  SBC R_DX+1
  STA R_DX+1
ss_dxchk:
  LDA R_DX+1
  BEQ ss_dxok
  JMP ss_rnext
ss_dxok:
  LDA #2
  CLC
  ADC R_RAD
  CMP R_DX
  BCC ss_rnext
  BEQ ss_rnext
  // dy the same way.
  LDY R_T0
  LDA rock_y, Y
  CLC
  ADC R_RAD
  STA R_DX
  LDA rock_y+1, Y
  ADC #0
  STA R_DX+1
  LDY R_OFF
  LDA shot_y, Y
  CLC
  ADC #4
  SEC
  SBC R_DX
  STA R_DX
  LDA shot_y+1, Y
  ADC #0
  SBC R_DX+1
  STA R_DX+1
  LDA R_DX+1
  BPL ss_dychk
  LDA #0
  SEC
  SBC R_DX
  STA R_DX
  LDA #0
  SBC R_DX+1
  STA R_DX+1
ss_dychk:
  LDA R_DX+1
  BEQ ss_dyok
  JMP ss_rnext
ss_dyok:
  LDA #2
  CLC
  ADC R_RAD
  CMP R_DX
  BCC ss_rnext
  BEQ ss_rnext
  // Hit: shot dies, victim recorded; beat_shots splits the rock.
  LDY R_IDX
  LDA #0
  STA shot_on, Y
  TXA
  STA as_shot_victim, Y
  LDA ss_bits, Y
  ORA as_shot_dead
  STA as_shot_dead
  JMP ss_next
ss_rnext:
  INX
  CPX #AS_ROCKS
  BCS ss_paint
  JMP ss_rloop
// Surviving shot: repaint if its column, row or phase moved (the C++
// shot_present chain per shot per tick cost more than the strips).
ss_paint:
  LDY R_OFF
  LDA shot_x, Y
  LSR A
  LSR A
  STA R_COL
  LDA shot_x+1, Y
  AND #1
  BEQ ssp_col0
  LDA R_COL
  CLC
  ADC #64
  STA R_COL
ssp_col0:
  LDY R_OFF
  LDA shot_y, Y
  STA R_SY
  LDA shot_x, Y
  AND #3
  STA R_PH
  LDY R_IDX
  LDA shot_shown, Y
  BEQ ssp_draw
  LDA R_COL
  CMP shot_col, Y
  BNE ssp_wipe
  LDA R_SY
  CMP shot_sy, Y
  BNE ssp_wipe
  LDA R_PH
  CMP shot_phase, Y
  BEQ ss_next
ssp_wipe:
  LDA shot_col, Y
  STA as_col
  LDA shot_sy, Y
  STA as_y
  JSR shot_wipe
  LDY R_IDX
ssp_draw:
  LDA R_COL
  STA shot_col, Y
  STA as_col
  LDA R_SY
  STA shot_sy, Y
  STA as_y
  LDA R_PH
  STA shot_phase, Y
  ASL A
  TAY
  LDA shot_img_tab, Y
  STA as_src
  LDA shot_img_tab+1, Y
  STA as_src+1
  JSR shot_draw
  LDY R_IDX
  LDA #1
  STA shot_shown, Y
  JMP ss_next
ss_expire:
  LDY R_IDX
  LDA #0
  STA shot_on, Y
  LDA ss_bits, Y
  ORA as_shot_dead
  STA as_shot_dead
ss_next:
  INC R_IDX
  LDA R_IDX
  CMP #AS_SHOTS
  BCS ss_ret
  JMP ss_loop
ss_ret:
  RTS

// ---------------------------------------------------------------- blit

// rock_blit: wipe a rock's old window and draw its new one under a
// single shadow mapping, walking byte columns by +8 instead of
// recomputing a strip address (and toggling ACCCON) per column the way
// the soft_* helpers do. Inputs: as_col / as_y / as_src for the draw,
// B_OLDX / B_OLDY for the wipe (skipped when B_WIPE is 0), B_COLS and
// B_ROWS for the window size. FX_DST / FX_SRC (0x70-0x73) are screen1's
// temps; only shadow_on/off run inside, and they touch neither.
blit_row_lo:
.byte 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80
.byte 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80
.byte 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80
.byte 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80
blit_row_hi:
.byte 0x30, 0x32, 0x35, 0x37, 0x3a, 0x3c, 0x3f, 0x41
.byte 0x44, 0x46, 0x49, 0x4b, 0x4e, 0x50, 0x53, 0x55
.byte 0x58, 0x5a, 0x5d, 0x5f, 0x62, 0x64, 0x67, 0x69
.byte 0x6c, 0x6e, 0x71, 0x73, 0x76, 0x78, 0x7b, 0x7d

// B_BASE = cell base of (A = byte column, B_TMPY = pixel row);
// B_SCAN = scanline within the cell.
rb_addr:
  STA B_CNT
  LDA B_TMPY
  LSR A
  LSR A
  LSR A
  TAX
  LDA #0
  STA B_BASE+1
  LDA B_CNT
  ASL A
  ROL B_BASE+1
  ASL A
  ROL B_BASE+1
  ASL A
  ROL B_BASE+1
  STA B_BASE
  LDA B_TMPY
  AND #7
  STA B_SCAN
  CLC
  LDA B_BASE
  ADC blit_row_lo, X
  STA B_BASE
  STA 0x70
  LDA B_BASE+1
  ADC blit_row_hi, X
  STA B_BASE+1
  STA 0x71
  RTS

rb_next_col:
  CLC
  LDA B_BASE
  ADC #8
  STA B_BASE
  STA 0x70
  BCC rb_nc1
  INC B_BASE+1
rb_nc1:
  LDA B_BASE+1
  STA 0x71
  RTS

.global rock_blit
rock_blit:
  LDA B_WIPE
  BNE rb_wipepass
  JMP rb_drawpass
rb_wipepass:
  LDA B_OLDY
  STA B_TMPY
  LDA B_OLDX
  JSR rb_addr
  JSR shadow_on
  LDA B_COLS
  STA B_NCOL
rb_wcol:
  LDX #0
  LDY B_SCAN
  BEQ rb_wchk
  // First segment: up to the char-row boundary (or all rows).
  LDA #8
  SEC
  SBC B_SCAN
  CMP B_ROWS
  BCC rb_wseg1
  LDA B_ROWS
rb_wseg1:
  STA B_CNT
rb_wseg:
  LDA #0
  STA (0x70), Y
  INX
  INY
  DEC B_CNT
  BNE rb_wseg
  JMP rb_wnext
// Full char rows: 8 bytes, no per-byte checks.
rb_wfull:
  LDY #0
  LDA #0
  STA (0x70), Y
  INY
  STA (0x70), Y
  INY
  STA (0x70), Y
  INY
  STA (0x70), Y
  INY
  STA (0x70), Y
  INY
  STA (0x70), Y
  INY
  STA (0x70), Y
  INY
  STA (0x70), Y
  TXA
  CLC
  ADC #8
  TAX
rb_wnext:
  // Advance to the next char row (+0x280) between segments. Only the
  // 0x70 pointer moves: rb_next_col derives the next column from
  // B_BASE, which must stay at this column's top.
  CPX B_ROWS
  BCS rb_wcol_done
  LDA 0x70
  CLC
  ADC #0x80
  STA 0x70
  LDA 0x71
  ADC #2
  STA 0x71
rb_wchk:
  STX B_CNT
  LDA B_ROWS
  SEC
  SBC B_CNT
  STA B_CNT
  CMP #8
  BCS rb_wfull
  LDY #0
  JMP rb_wseg
rb_wcol_done:
  JSR rb_next_col
  DEC B_NCOL
  BNE rb_wcol
  JSR shadow_off
rb_drawpass:
  // as_* live in high BSS inside the shadow window, so they must all
  // be read before shadow_on maps that window over them.
  LDA as_y
  STA B_TMPY
  LDA as_src
  STA 0x72
  LDA as_src+1
  STA 0x73
  LDA as_col
  JSR rb_addr
  JSR shadow_on
  LDA B_COLS
  STA B_NCOL
rb_dcol:
  LDA 0x72
  STA rb_src_op+1
  LDA 0x73
  STA rb_src_op+2
  LDX #0
  LDY B_SCAN
  BEQ rb_dchk
  // First segment: up to the char-row boundary (or all rows).
  LDA #8
  SEC
  SBC B_SCAN
  CMP B_ROWS
  BCC rb_dseg1
  LDA B_ROWS
rb_dseg1:
  STA B_CNT
rb_dseg:
rb_src_op:
  LDA 0xffff, X
  STA (0x70), Y
  INX
  INY
  DEC B_CNT
  BNE rb_dseg
  JMP rb_dnext
// Full char rows: 8 bytes through the zp source pointer, no checks.
rb_dfull:
  LDA rb_src_op+1
  STX B_CNT
  CLC
  ADC B_CNT
  STA 0x72
  LDA rb_src_op+2
  ADC #0
  STA 0x73
  LDY #0
  LDA (0x72), Y
  STA (0x70), Y
  INY
  LDA (0x72), Y
  STA (0x70), Y
  INY
  LDA (0x72), Y
  STA (0x70), Y
  INY
  LDA (0x72), Y
  STA (0x70), Y
  INY
  LDA (0x72), Y
  STA (0x70), Y
  INY
  LDA (0x72), Y
  STA (0x70), Y
  INY
  LDA (0x72), Y
  STA (0x70), Y
  INY
  LDA (0x72), Y
  STA (0x70), Y
  TXA
  CLC
  ADC #8
  TAX
rb_dnext:
  // Advance to the next char row (+0x280) between segments. Only the
  // 0x70 pointer moves: rb_next_col derives the next column from
  // B_BASE, which must stay at this column's top.
  CPX B_ROWS
  BCS rb_dcol_done
  LDA 0x70
  CLC
  ADC #0x80
  STA 0x70
  LDA 0x71
  ADC #2
  STA 0x71
rb_dchk:
  STX B_CNT
  LDA B_ROWS
  SEC
  SBC B_CNT
  STA B_CNT
  CMP #8
  BCS rb_dfull
  LDY #0
  JMP rb_dseg
rb_dcol_done:
  LDA rb_src_op+1
  CLC
  ADC B_ROWS
  STA 0x72
  LDA rb_src_op+2
  ADC #0
  STA 0x73
rb_dc1:
  JSR rb_next_col
  DEC B_NCOL
  BEQ rb_ddone
  JMP rb_dcol
rb_ddone:
  JMP shadow_off

// Per-tick ship paint: beat_ship calls this instead of the C++
// ship_present, whose compiled decision + call chain cost ~8K cycles
// — enough to spill a tick on its own. Paints when the ship turned,
// moved or changed thrust frame; when idle it still repaints every
// other call, because a rock/shot wipe crossing a stationary ship
// (common while it sits invulnerable after respawn) would otherwise
// leave it permanently half-erased. ship_present (C++) keeps the
// same bookkeeping for ship_reset.
ship_img_tab:
  .hword k_as_ship0
  .hword k_as_ship1
  .hword k_as_ship2
  .hword k_as_ship3
  .hword k_as_ship4
  .hword k_as_ship5
  .hword k_as_ship6
  .hword k_as_ship7
  .hword k_as_ship_t0
  .hword k_as_ship_t1
  .hword k_as_ship_t2
  .hword k_as_ship_t3
  .hword k_as_ship_t4
  .hword k_as_ship_t5
  .hword k_as_ship_t6
  .hword k_as_ship_t7

ship_repair:
.byte 0

.global ship_paint
ship_paint:
  // ncol = ship_x >> 2 (ship_y < 256, so sy is its low byte).
  LDA ship_x
  LSR A
  LSR A
  STA R_COL
  LDA ship_x+1
  AND #1
  BEQ sp_col0
  LDA R_COL
  CLC
  ADC #64
  STA R_COL
sp_col0:
  LDA ship_shown
  BEQ sp_paint
  LDA R_COL
  CMP ship_px
  BNE sp_paint
  LDA ship_y
  CMP ship_py
  BNE sp_paint
  LDA ship_dir
  CMP ship_dir_shown
  BNE sp_paint
  LDA ship_thrusting
  CMP ship_thrust_shown
  BNE sp_paint
  // Unchanged: repaint only on alternate calls (wipe repair).
  LDA ship_repair
  EOR #1
  STA ship_repair
  BEQ sp_skip
sp_paint:
  // as_col/as_y are the new window. ship_px/ship_py stay on the
  // previously drawn window until after the blit: ship_blit wipes
  // those, and storing the new cell first erased the destination
  // and left the old exhaust (and any trailing hull pixels) behind.
  LDA R_COL
  STA as_col
  LDA ship_y
  STA as_y
  LDA ship_dir
  STA ship_dir_shown
  LDA ship_thrusting
  STA ship_thrust_shown
  ASL A
  ASL A
  ASL A
  CLC
  ADC ship_dir
  ASL A
  TAY
  LDA ship_img_tab, Y
  STA as_src
  LDA ship_img_tab+1, Y
  STA as_src+1
  JSR ship_blit
  LDA R_COL
  STA ship_px
  LDA ship_y
  STA ship_py
  LDA #1
  STA ship_shown
sp_skip:
  RTS

// Ship repaint through rock_blit: the ship art is 4x16 column-major
// like a rock window, and ship_px/ship_py/ship_shown still hold the
// previously painted position. Caller sets as_col/as_y/as_src.
.global ship_blit
ship_blit:
  LDA ship_px
  STA B_OLDX
  LDA ship_py
  STA B_OLDY
  LDA ship_shown
  STA B_WIPE
  LDA #AS_SOFT_SHIP_COLS
  STA B_COLS
  LDA #AS_SOFT_SHIP_ROWS
  STA B_ROWS
  JMP rock_blit

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
