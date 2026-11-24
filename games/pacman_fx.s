// Pac-Man blitters & hardware helpers for Shadow Mode 2.
// Lives in .text.blitter below &3000 alongside screen1.s.

#include "pacman.h"

.set OSBYTE 0xfff4
.set SYSVIA_LATCH 0xfe40
.set SYSVIA_DDRA 0xfe43
.set SYSVIA_PORTA 0xfe4f

.set FX_DST 0x70
.set E_T0 0x78
.set E_T1 0x79
.set E_C 0x7a
.set E_R 0x7b
.set E_IDX 0x7c
.set E_CNT 0x7d
.set E_B 0x7e
.set W_SX 0x7f
.set W_SY 0x80
.set W_COLS 0x81
.set W_ROWS 0x82
.set W_RELX 0x83
.set W_BY0 0x84
.set W_SC 0x85

.section ".text.blitter", "ax", @progbits

col_off_tab:
  .byte 0, 24, 48, 72, 96, 120, 144, 168

comp_buf:
  .space 192

ar_idx:
  .byte 0
done_n:
  .byte 0
done_k0:
  .space 6
done_k1:
  .space 6
prev_sx:
  .space 5
prev_sy:
  .space 5
prev_lo:
  .space 5
prev_hi:
  .space 5

tile_y_tab:
  .byte 16, 28, 40, 52, 64, 76, 88, 100, 112, 124
  .byte 136, 148, 160, 172, 184, 196, 208, 220, 232

row_off_lo:
  .byte 0, 19, 38, 57, 76, 95, 114, 133, 152, 171
  .byte 190, 209, 228, 247, 10, 29, 48, 67, 86

row_off_hi:
  .byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
  .byte 0, 0, 0, 0, 1, 1, 1, 1, 1

.global frame_wait
frame_wait:
  LDA #19
  JMP OSBYTE

// 5-digit score + wave + lives on character row 0 using Mode-2 digits.
.global hud_digits
hud_digits:
  LDA score
  STA E_T0
  LDA score+1
  STA E_T1
  LDA #HUD_SCORE_COL
  STA E_C
  LDX #0
hd_place:
  LDY #0
hd_sub:
  SEC
  LDA E_T0
  SBC hud_places, X
  STA E_R
  LDA E_T1
  SBC hud_places+1, X
  BCC hd_digit
  STA E_T1
  LDA E_R
  STA E_T0
  INY
  BNE hd_sub
hd_digit:
  STX E_IDX
  TYA
  JSR hud_glyph
  LDX E_IDX
  INX
  INX
  CPX #10
  BNE hd_place
  LDA wave
  LDX #HUD_WAVE_COL
  JSR hud_unit
  LDA lives
  LDX #HUD_LIVES_COL
hud_unit:
  STX E_C
  CMP #10
  BCC hud_glyph
  LDA #9
hud_glyph:
  LDY #0
  STY fx_src+1
  ASL A
  ROL fx_src+1
  ASL A
  ROL fx_src+1
  ASL A
  ROL fx_src+1
  ASL A
  ROL fx_src+1
  ASL A
  ROL fx_src+1
  CLC
  ADC #%lo(k_pm_digits)
  STA fx_src
  LDA fx_src+1
  ADC #%hi(k_pm_digits)
  STA fx_src+1
  LDA E_C
  STA fx_col
  CLC
  ADC #4
  STA E_C
  LDA #HUD_ROW
  STA fx_row
  LDA #32
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

tile_coords:
  LDA pm_col
  ASL A
  ASL A
  CLC
  ADC #PM_ORIGIN_COL
  STA fx_col
  LDX pm_row
  LDA tile_y_tab, X
  STA fx_y
  RTS

// Opaque 4x12 tile blit from pm_src at (pm_col, pm_row).
.global cell_draw
cell_draw:
  JSR tile_coords
  LDA pm_src
  STA fx_src
  LDA pm_src+1
  STA fx_src+1
  LDA #PM_TILE_COLS
  STA E_CNT
cd_col:
  LDA #PM_TILE_H
  STA fx_len
  JSR fx_strip
  INC fx_col
  CLC
  LDA fx_src
  ADC #PM_TILE_H
  STA fx_src
  BCC cd_nc
  INC fx_src+1
cd_nc:
  DEC E_CNT
  BNE cd_col
  RTS

.global cell_wipe
cell_wipe:
  JSR tile_coords
  LDA #PM_TILE_COLS
  STA E_CNT
cw_col:
  LDA #PM_TILE_H
  STA fx_len
  JSR fx_blank
  INC fx_col
  DEC E_CNT
  BNE cw_col
  RTS

// Look up maze[pm_row * 19 + pm_col].
// Carry clear if empty/void (< 2); Carry set with pm_src & E_T0/E_T1 = 48B art.
tile_lookup:
  LDX pm_row
  CLC
  LDA row_off_lo, X
  ADC pm_col
  STA E_T0
  LDA row_off_hi, X
  ADC #0
  STA E_T1
  CLC
  LDA E_T0
  ADC #%lo(maze)
  STA E_T0
  LDA E_T1
  ADC #%hi(maze)
  STA E_T1
  LDY #0
  LDA (E_T0), Y
  CMP #2
  BCS tl_art
  RTS
tl_art:
  CMP #PM_PELLET
  BEQ tl_pellet
  CMP #PM_POWER
  BEQ tl_power
  CMP #PM_GATE
  BEQ tl_gate
  AND #15
  ASL A
  ASL A
  ASL A
  ASL A
  STA E_T0
  LDA #0
  STA E_T1
  LDA E_T0
  ASL A
  ROL E_T1
  CLC
  ADC E_T0
  BCC tl_wnc
  INC E_T1
tl_wnc:
  CLC
  ADC #%lo(k_pm_wall)
  STA E_T0
  STA pm_src
  LDA E_T1
  ADC #%hi(k_pm_wall)
  STA E_T1
  STA pm_src+1
  SEC
  RTS
tl_pellet:
  LDA #%lo(k_pm_pellet)
  LDX #%hi(k_pm_pellet)
  JMP tl_set
tl_power:
  LDA #%lo(k_pm_power)
  LDX #%hi(k_pm_power)
  JMP tl_set
tl_gate:
  LDA #%lo(k_pm_gate)
  LDX #%hi(k_pm_gate)
tl_set:
  STA E_T0
  STA pm_src
  STX E_T1
  STX pm_src+1
  SEC
  RTS

.global tile_draw
tile_draw:
  JSR tile_lookup
  BCS td_art
  JMP cell_wipe
td_art:
  JMP cell_draw

// Per-pixel transparent Mode-2 4x12 sprite blit at sub-tile (pm_sx, pm_sy).
.global actor_draw
actor_draw:
  LDA pm_sx
  CLC
  ADC #PM_ORIGIN_COL
  STA fx_col
  LDA pm_sy
  ASL A
  ADC pm_sy
  ADC #PM_ORIGIN_Y
  STA fx_y
  LDA pm_src
  STA E_T0
  LDA pm_src+1
  STA E_T1
  LDA #PM_TILE_COLS
  STA E_CNT
ad_col:
  JSR mstrip2
  INC fx_col
  CLC
  LDA E_T0
  ADC #PM_TILE_H
  STA E_T0
  BCC ad_nc
  INC E_T1
ad_nc:
  DEC E_CNT
  BNE ad_col
  RTS

mstrip2:
  JSR strip_addr
  JSR shadow_on
  STY E_R
  LDX #0
ms_loop:
  TXA
  TAY
  LDA (E_T0), Y
  BEQ ms_skip
  STA E_B
  LDY E_R
  AND #0xaa
  BEQ ms_keep_l
  LDA E_B
  AND #0x55
  BEQ ms_keep_r
  LDA E_B
  STA (FX_DST), Y
  JMP ms_skip
ms_keep_l:
  LDA (FX_DST), Y
  AND #0xaa
  ORA E_B
  STA (FX_DST), Y
  JMP ms_skip
ms_keep_r:
  LDA (FX_DST), Y
  AND #0x55
  ORA E_B
  STA (FX_DST), Y
ms_skip:
  INX
  CPX #PM_TILE_H
  BEQ ms_done
  LDY E_R
  INY
  STY E_R
  CPY #8
  BNE ms_loop
  JSR next_char_row
  STY E_R
  JMP ms_loop
ms_done:
  JMP shadow_off

sound_play:
  LDA #7
  JMP 0xfff1

.global sound_chomp
sound_chomp:
  LDX #%lo(snd_chomp)
  LDY #%hi(snd_chomp)
  JMP sound_play

.global sound_death
sound_death:
  LDX #%lo(snd_death)
  LDY #%hi(snd_death)
  JMP sound_play

.global sound_eat
sound_eat:
  LDX #%lo(snd_eat)
  LDY #%hi(snd_eat)
  JMP sound_play

.global maze_reset
maze_reset:
  LDX #0
mr_p0:
  LDA maze_rom, X
  STA maze, X
  INX
  BNE mr_p0
  LDX #104
mr_p1:
  LDA maze_rom+256, X
  STA maze+256, X
  DEX
  BPL mr_p1
  RTS

.global paint_rows
paint_rows:
pr_row:
  LDA #0
  STA pm_col
pr_col:
  JSR tile_draw
  INC pm_col
  LDA pm_col
  CMP #PM_COLS
  BCC pr_col
  INC pm_row
  LDA pm_end_row
  CMP pm_row
  BCS pr_row
  RTS

.global footprint_restore
footprint_restore:
  LDA pm_sx
  LSR A
  LSR A
  STA E_C
  STA pm_col
  LDA pm_sy
  LSR A
  LSR A
  STA E_IDX
  STA pm_row
  JSR tile_draw
  LDA pm_sx
  AND #3
  BEQ fr_ny
  LDX E_C
  INX
  CPX #PM_COLS
  BCS fr_ny
  STX pm_col
  LDA E_IDX
  STA pm_row
  JSR tile_draw
fr_ny:
  LDA pm_sy
  AND #3
  BEQ fr_done
  LDX E_IDX
  INX
  CPX #PM_ROWS
  BCS fr_done
  STX pm_row
  LDA E_C
  STA pm_col
  JMP tile_draw
fr_done:
  RTS

// Offscreen-composited 1x1..2x2 tile window blit: renders maze tiles + all
// overlapping actors into comp_buf in RAM, then writes each column to shadow
// VRAM in a single fx_strip pass so sprites NEVER flicker or erase each other.
.global patch_window
patch_window:
  LDA pm_wx
  CMP #PM_COLS
  BCS pw_abort
  LDA pm_wy
  CMP #PM_ROWS
  BCC pw_ok_xy
pw_abort:
  RTS
pw_ok_xy:
  CLC
  LDA pm_wx
  ADC pm_wc
  CMP #PM_COLS+1
  BCC pw_ok_wc
  LDA #1
  STA pm_wc
pw_ok_wc:
  CLC
  LDA pm_wy
  ADC pm_wr
  CMP #PM_ROWS+1
  BCC pw_ok_wr
  LDA #1
  STA pm_wr
pw_ok_wr:
  LDA pm_wx
  ASL A
  ASL A
  STA W_SX
  LDA pm_wy
  ASL A
  ASL A
  STA W_SY
  LDA pm_wc
  ASL A
  ASL A
  STA W_COLS
  LDX #12
  LDA pm_wr
  CMP #1
  BEQ pw_set_rows
  LDX #24
pw_set_rows:
  STX W_ROWS

  // Step 1: fill comp_buf from pm_wc x pm_wr maze tiles.
  LDA #0
  STA E_R
  STA E_B
pw_tr_loop:
  CLC
  LDA pm_wy
  ADC E_R
  STA pm_row
  LDA #0
  STA E_IDX
pw_tc_loop:
  CLC
  LDA pm_wx
  ADC E_IDX
  STA pm_col
  LDA E_IDX
  ASL A
  ASL A
  STA E_C
  JSR tile_lookup
  BCS pw_tile_copy
  LDA #4
  STA W_SC
pw_tz_col:
  LDX E_C
  LDA col_off_tab, X
  CLC
  ADC E_B
  TAX
  LDY #12
  LDA #0
pw_tz_row:
  STA comp_buf, X
  INX
  DEY
  BNE pw_tz_row
  INC E_C
  DEC W_SC
  BNE pw_tz_col
  JMP pw_tc_next

pw_tile_copy:
  LDY #0
  LDA #4
  STA W_SC
pw_tc_col:
  LDX E_C
  LDA col_off_tab, X
  CLC
  ADC E_B
  TAX
  LDA #12
  STA E_CNT
pw_tc_row:
  LDA (E_T0), Y
  STA comp_buf, X
  INX
  INY
  DEC E_CNT
  BNE pw_tc_row
  INC E_C
  DEC W_SC
  BNE pw_tc_col

pw_tc_next:
  INC E_IDX
  LDA E_IDX
  CMP pm_wc
  BCC pw_tc_loop
  LDA #12
  STA E_B
  INC E_R
  LDA E_R
  CMP pm_wr
  BCC pw_tr_loop

  // Step 2: composite overlapping actors (0..3 ghosts, 4 Pac-Man on top).
  LDA #0
  STA E_IDX
pw_act_loop:
  LDX E_IDX
  LDA act_spr_hi, X
  BNE pw_act_vis
  JMP pw_act_next
pw_act_vis:
  STA E_T1
  LDA act_spr_lo, X
  STA E_T0
  SEC
  LDA act_sx, X
  SBC W_SX
  STA W_RELX
  CLC
  ADC #3
  STA E_B
  LDA W_COLS
  CLC
  ADC #3
  CMP E_B
  BEQ pw_act_skip
  BCS pw_act_xok
pw_act_skip:
  JMP pw_act_next
pw_act_xok:
  SEC
  LDA act_sy, X
  SBC W_SY
  CLC
  ADC #3
  STA E_B
  LDA pm_wr
  ASL A
  ASL A
  CLC
  ADC #3
  CMP E_B
  BEQ pw_act_skip
  BCC pw_act_skip
  // W_BY0 = (rel_sy + 3) * 3 - 9
  LDA E_B
  ASL A
  CLC
  ADC E_B
  SEC
  SBC #9
  STA W_BY0

  LDA #0
  STA W_SC
  TAY
pw_sc_loop:
  CLC
  LDA W_RELX
  ADC W_SC
  CMP W_COLS
  BCC pw_sc_in
  TYA
  CLC
  ADC #12
  TAY
  JMP pw_sc_next
pw_sc_in:
  TAX
  LDA col_off_tab, X
  STA E_C
  LDA W_BY0
  STA E_R
  LDA #12
  STA E_CNT
pw_sr_loop:
  LDA E_R
  CMP W_ROWS
  BCS pw_px_skip
  LDA (E_T0), Y
  BEQ pw_px_skip
  STA E_B
  LDA E_C
  CLC
  ADC E_R
  TAX
  LDA E_B
  AND #0xaa
  BEQ pw_keep_l
  LDA E_B
  AND #0x55
  BEQ pw_keep_r
  LDA E_B
  STA comp_buf, X
  JMP pw_px_skip
pw_keep_l:
  LDA comp_buf, X
  AND #0xaa
  ORA E_B
  STA comp_buf, X
  JMP pw_px_skip
pw_keep_r:
  LDA comp_buf, X
  AND #0x55
  ORA E_B
  STA comp_buf, X
pw_px_skip:
  INY
  INC E_R
  DEC E_CNT
  BNE pw_sr_loop
pw_sc_next:
  INC W_SC
  LDA W_SC
  CMP #4
  BCC pw_sc_loop

pw_act_next:
  INC E_IDX
  LDA E_IDX
  CMP #5
  BCS pw_blit_start
  JMP pw_act_loop

  // Step 3: blit comp_buf to shadow VRAM in one atomic pass per column.
pw_blit_start:
  LDA W_SX
  CLC
  ADC #PM_ORIGIN_COL
  STA fx_col
  LDX pm_wy
  LDA tile_y_tab, X
  STA fx_y
  LDA W_ROWS
  STA fx_len
  LDA #%lo(comp_buf)
  STA fx_src
  LDA #%hi(comp_buf)
  STA fx_src+1
  LDA W_COLS
  STA E_CNT
pw_blit_col:
  JSR fx_strip
  INC fx_col
  CLC
  LDA fx_src
  ADC #24
  STA fx_src
  BCC pw_blit_nc
  INC fx_src+1
pw_blit_nc:
  DEC E_CNT
  BNE pw_blit_col
  RTS

// Store (pm_sx, pm_sy, pm_src) into act_*[pm_g].
.global act_store
act_store:
  LDX pm_g
  LDA pm_sx
  STA act_sx, X
  LDA pm_sy
  STA act_sy, X
  LDA pm_src
  STA act_spr_lo, X
  LDA pm_src+1
  STA act_spr_hi, X
  RTS

// Deduplicated patch_window emit for (pm_wx, pm_wy, pm_wc, pm_wr).
emit_win_asm:
  LDA pm_wy
  ASL A
  ASL A
  LDX pm_wc
  CPX #2
  BNE ew_c1
  ORA #2
ew_c1:
  LDX pm_wr
  CPX #2
  BNE ew_r1
  ORA #1
ew_r1:
  STA E_B
  LDX done_n
  BEQ ew_add
ew_chk:
  DEX
  LDA done_k0, X
  CMP pm_wx
  BNE ew_nxt
  LDA done_k1, X
  CMP E_B
  BNE ew_nxt
  RTS
ew_nxt:
  TXA
  BNE ew_chk
ew_add:
  LDX done_n
  CPX #6
  BCS ew_go
  LDA pm_wx
  STA done_k0, X
  LDA E_B
  STA done_k1, X
  INC done_n
ew_go:
  JMP patch_window

// Emit spot covering 4x12 sprite at (pm_sx, pm_sy).
emit_spot_asm:
  LDA pm_sx
  LSR A
  LSR A
  STA pm_wx
  LDA pm_sy
  LSR A
  LSR A
  STA pm_wy
  LDX #1
  LDA pm_sx
  AND #3
  BEQ es_c1
  INX
es_c1:
  STX pm_wc
  LDX #1
  LDA pm_sy
  AND #3
  BEQ es_r1
  INX
es_r1:
  STX pm_wr
  JMP emit_win_asm

.global actors_snap
actors_snap:
  LDX #4
as_loop:
  LDA act_sx, X
  STA prev_sx, X
  LDA act_sy, X
  STA prev_sy, X
  LDA act_spr_lo, X
  STA prev_lo, X
  LDA act_spr_hi, X
  STA prev_hi, X
  DEX
  BPL as_loop
  LDA #0
  STA pac_dirty
  RTS

.global actors_draw
actors_draw:
  LDA #0
  STA done_n
  STA ar_idx
adw_loop:
  LDX ar_idx
  LDA act_sx, X
  STA pm_sx
  LDA act_sy, X
  STA pm_sy
  JSR emit_spot_asm
  INC ar_idx
  LDA ar_idx
  CMP #5
  BCC adw_loop
  JMP actors_snap

.global actors_refresh
actors_refresh:
  LDA #0
  STA done_n
  STA ar_idx
arf_loop:
  LDX ar_idx
  LDA act_sx, X
  CMP prev_sx, X
  BNE arf_changed
  LDA act_sy, X
  CMP prev_sy, X
  BNE arf_changed
  LDA act_spr_lo, X
  CMP prev_lo, X
  BNE arf_changed
  LDA act_spr_hi, X
  CMP prev_hi, X
  BNE arf_changed
  CPX #4
  BNE arf_skip
  LDA pac_dirty
  BNE arf_changed
arf_skip:
  JMP arf_next

arf_changed:
  // Check |nsx - osx| <= 1 and |nsy - osy| <= 1: (u8)(n - o + 1) < 3.
  SEC
  LDA act_sx, X
  SBC prev_sx, X
  CLC
  ADC #1
  CMP #3
  BCS arf_split
  SEC
  LDA act_sy, X
  SBC prev_sy, X
  CLC
  ADC #1
  CMP #3
  BCS arf_split

  // Combined minimal 1x1..2x2 window covering old and new.
  LDA prev_sx, X
  CMP act_sx, X
  BCC arf_xord
  LDA act_sx, X
  LDY prev_sx, X
  JMP arf_xset
arf_xord:
  LDY act_sx, X
arf_xset:
  LSR A
  LSR A
  STA pm_wx
  TYA
  CLC
  ADC #3
  LSR A
  LSR A
  SEC
  SBC pm_wx
  CLC
  ADC #1
  STA pm_wc

  LDA prev_sy, X
  CMP act_sy, X
  BCC arf_yord
  LDA act_sy, X
  LDY prev_sy, X
  JMP arf_yset
arf_yord:
  LDY act_sy, X
arf_yset:
  LSR A
  LSR A
  STA pm_wy
  TYA
  CLC
  ADC #3
  LSR A
  LSR A
  SEC
  SBC pm_wy
  CLC
  ADC #1
  STA pm_wr
  JSR emit_win_asm
  JMP arf_next

arf_split:
  LDA prev_sx, X
  STA pm_sx
  LDA prev_sy, X
  STA pm_sy
  JSR emit_spot_asm
  LDX ar_idx
  LDA act_sx, X
  STA pm_sx
  LDA act_sy, X
  STA pm_sy
  JSR emit_spot_asm

arf_next:
  INC ar_idx
  LDA ar_idx
  CMP #5
  BCS arf_done
  JMP arf_loop
arf_done:
  JMP actors_snap

.global actors_wipe_all
actors_wipe_all:
  LDA #0
  STA ar_idx
awa_loop:
  LDX ar_idx
  LDA prev_sx, X
  STA pm_sx
  LDA prev_sy, X
  STA pm_sy
  JSR footprint_restore
  LDX ar_idx
  LDA act_sx, X
  STA pm_sx
  LDA act_sy, X
  STA pm_sy
  JSR footprint_restore
  INC ar_idx
  LDA ar_idx
  CMP #5
  BCC awa_loop
  RTS

.global ghost_load
ghost_load:
  LDX pm_g
  LDA ghost_x, X
  STA gx
  LDA ghost_y, X
  STA gy
  LDA ghost_sx, X
  STA gsx
  LDA ghost_sy, X
  STA gsy
  LDA ghost_dir, X
  STA gdir
  LDA ghost_mode, X
  STA gmode
  LDA ghost_house, X
  STA ghouse
  LDA ghost_wait, X
  STA gwait
  RTS

.global ghost_save
ghost_save:
  LDX pm_g
  LDA gx
  STA ghost_x, X
  LDA gy
  STA ghost_y, X
  LDA gsx
  STA ghost_sx, X
  LDA gsy
  STA ghost_sy, X
  LDA gdir
  STA ghost_dir, X
  LDA gmode
  STA ghost_mode, X
  LDA ghouse
  STA ghost_house, X
  LDA gwait
  STA ghost_wait, X
  RTS

// Pre-autotiled 19x19 classic Pac-Man maze (0 dead ends, 165 pellets).
maze_rom:
  .byte 22, 26, 26, 26, 26, 26, 26, 26, 26, 30, 26, 26, 26, 26, 26, 26, 26, 26, 28  // ###################
  .byte 21, 2, 2, 2, 2, 2, 2, 2, 2, 21, 2, 2, 2, 2, 2, 2, 2, 2, 21  // #........#........#
  .byte 21, 3, 18, 24, 2, 18, 26, 24, 2, 17, 2, 18, 26, 24, 2, 18, 24, 3, 21  // #o##.###.#.###.##o#
  .byte 21, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 21  // #.................#
  .byte 21, 2, 18, 24, 2, 20, 2, 18, 26, 30, 26, 24, 2, 20, 2, 18, 24, 2, 21  // #.##.#.#####.#.##.#
  .byte 21, 2, 2, 2, 2, 21, 2, 2, 2, 21, 2, 2, 2, 21, 2, 2, 2, 2, 21  // #....#...#...#....#
  .byte 19, 26, 26, 28, 2, 23, 26, 24, 2, 17, 2, 18, 26, 29, 2, 22, 26, 26, 25  // ####.###.#.###.####
  .byte 1, 1, 1, 21, 2, 21, 2, 2, 2, 2, 2, 2, 2, 21, 2, 21, 1, 1, 1  // xxx#.#.......#.#xxx
  .byte 18, 26, 26, 25, 2, 17, 2, 22, 24, 4, 18, 28, 2, 17, 2, 19, 26, 26, 24  // ####.#.##=##.#.####
  .byte 0, 0, 0, 2, 2, 2, 2, 21, 0, 0, 0, 21, 2, 2, 2, 2, 0, 0, 0  //    ....#   #....   
  .byte 22, 26, 26, 24, 2, 16, 2, 19, 26, 30, 26, 25, 2, 16, 2, 18, 26, 26, 28  // ####.#.#####.#.####
  .byte 21, 2, 2, 2, 2, 2, 2, 2, 2, 21, 2, 2, 2, 2, 2, 2, 2, 2, 21  // #........#........#
  .byte 21, 2, 18, 28, 2, 18, 26, 24, 2, 17, 2, 18, 26, 24, 2, 22, 24, 2, 21  // #.##.###.#.###.##.#
  .byte 21, 3, 2, 21, 2, 2, 2, 2, 2, 0, 2, 2, 2, 2, 2, 21, 2, 3, 21  // #o.#.....P.....#.o#
  .byte 23, 24, 2, 17, 2, 20, 2, 18, 26, 30, 26, 24, 2, 20, 2, 17, 2, 18, 29  // ##.#.#.#####.#.#.##
  .byte 21, 2, 2, 2, 2, 21, 2, 2, 2, 21, 2, 2, 2, 21, 2, 2, 2, 2, 21  // #....#...#...#....#
  .byte 21, 2, 18, 26, 26, 27, 26, 24, 2, 17, 2, 18, 26, 27, 26, 26, 24, 2, 21  // #.######.#.######.#
  .byte 21, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 21  // #.................#
  .byte 19, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 25  // ###################
