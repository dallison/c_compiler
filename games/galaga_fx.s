// Galaga play for galaga.cc: each frame's keys, cannon, convoy sway, divers,
// dual lasers, bombs, and their collisions. Lives in .text.blitter so it
// stays below &3000 next to screen1.s. Units and layout are in galaga.h.
//
// Shadow Mode 2 (160x256, 8 colours, 2 px/byte). Both modes have 80 byte
// columns, so the fleet byte-column layout matches Galaxians; the horizontal
// pixel maths is halved (>>1, &1, *2) and all X are 8-bit (screen < 256 px).

#include "galaga.h"

.set OSWORD 0xfff1
.set OSBYTE 0xfff4
.set SYSVIA_LATCH 0xfe40
.set SYSVIA_DDRA 0xfe43
.set SYSVIA_PORTA 0xfe4f

.set FX_DST 0x70

.set E_NY 0x78
.set E_IDX 0x79
.set E_C 0x7c
.set E_R 0x7d
.set E_T0 0x7e
.set E_T1 0x7f
.set E_LEFT 0x80
.set E_C4 0x81
.set E_RANK 0x82
.set E_X 0x83
.set E_SLOT 0x85
.set E_SHOT 0x86

.section ".text.blitter", "ax", @progbits

shot_buf:
.space SHOT_SPAN

dv_col_buf:
.space 12

cur_col:
.byte 0
cur_y:
.byte 0
cur_slot:
.byte 0
fill_byte:
.byte 0

shot_masks:
.byte 0x2a, 0x15

// ---------------------------------------------------------------- misc

.global frame_wait
frame_wait:
  LDA #19
  JMP OSBYTE

// 4-digit score (clamped to 9999), wave, and lives on character row 0 using
// Mode-2 digits.
.global hud_digits
hud_digits:
  LDA score
  STA E_T0
  LDA score+1
  STA E_T1
  CMP #0x27
  BCC hd_counted
  BNE hd_clamp
  LDA E_T0
  CMP #0x10
  BCC hd_counted
hd_clamp:
  LDA #0x0f
  STA E_T0
  LDA #0x27
  STA E_T1
hd_counted:
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
  CPX #8
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
  ADC #%lo(k_ga_digits)
  STA fx_src
  LDA fx_src+1
  ADC #%hi(k_ga_digits)
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

// Two green frame lines across all 80 byte columns (top at LINE_Y, bottom
// at GROUND_Y). Screen::fill is Mode-1 only, so this fills in Mode 2.
.global hud_lines
hud_lines:
  LDA #0x0c            // Mode-2 byte for two green pixels
  STA fill_byte
  LDA #LINE_Y
  STA fx_y
  JSR band_row
  LDA #GROUND_Y
  STA fx_y
band_row:
  LDA #0
  STA fx_col
br_loop:
  LDA #2
  STA fx_len
  JSR fx_fill
  INC fx_col
  LDA fx_col
  CMP #80
  BNE br_loop
  RTS

// Fill fx_len scanlines down (fx_col, fx_y) with fill_byte.
fx_fill:
  JSR strip_addr
  JSR shadow_on
  LDX fx_len
ff_loop:
  LDA fill_byte
  STA (FX_DST), Y
  DEX
  BEQ ff_done
  INY
  CPY #8
  BNE ff_loop
  JSR next_char_row
  JMP ff_loop
ff_done:
  JMP shadow_off

// ---------------------------------------------------------------- keys

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
  LDA key_masks, X
  ORA key_bits
  STA key_bits
kr_up:
  DEX
  BPL kr_loop
  LDA #0x0b
  STA SYSVIA_LATCH
  CLI
  RTS

// ---------------------------------------------------------------- convoy

// Redraw up to FLEET_QUOTA live aliens at the current origin. Between sweeps
// fl_mover is 255; fleet_sway starts a new sweep after a step.
.global fleet_sway
fleet_sway:
  LDA alive
  BEQ fs_out
  LDA fl_mover
  CMP #255
  BNE fs_run
  JSR convoy_step
  LDA #0
  STA fl_mover
fs_run:
  LDA #FLEET_QUOTA
  STA E_LEFT
fs_loop:
  LDA fl_mover
  CMP #FLEET_SIZE
  BCS fs_done
  TAX
  LDA fl_on, X
  BEQ fs_next
  JSR alien_redraw
  DEC E_LEFT
  BEQ fs_out
fs_next:
  INC fl_mover
  BNE fs_loop
fs_done:
  LDA #255
  STA fl_mover
  LDA fl_frame
  EOR #1
  STA fl_frame
fs_out:
  RTS

// Nudge the convoy one byte column; reverse at the edges.
convoy_step:
  LDA fl_col
  STA fl_old_col
  LDA fl_row
  STA fl_old_row
  LDA fl_dir
  BMI cs_left
  LDX fl_hi
  LDA idx_col5, X
  CLC
  ADC fl_col
  ADC #5
  CMP #79
  BCS cs_rev_left
  INC fl_col
  RTS
cs_rev_left:
  LDA #255
  STA fl_dir
  RTS
cs_left:
  LDX fl_lo
  LDA idx_col5, X
  CLC
  ADC fl_col
  CMP #3
  BCC cs_rev_right
  DEC fl_col
  RTS
cs_rev_right:
  LDA #1
  STA fl_dir
  RTS

// X = alien. Draw at the current origin; the 6-column window's blank pads
// clear the trailing edge on a 1-column sway.
alien_redraw:
  LDA idx_rank, X
  STA E_RANK
  ASL A
  CLC
  ADC fl_row
  STA fx_row
  LDA idx_col5, X
  STA E_C4
  CLC
  ADC fl_col
  SEC
  SBC #1
  STA fx_col
  LDA E_RANK
  ASL A
  ORA fl_frame
  ASL A
  TAY
  LDA alien_spr, Y
  STA fx_src
  LDA alien_spr+1, Y
  STA fx_src+1
  LDA #48
  STA fx_len
  JMP fx_cell

// After a kill, refresh outermost live columns.
.global fleet_bounds
fleet_bounds:
  LDA #0
  STA fl_lo
fb_lo:
  LDX fl_lo
  JSR column_live
  BNE fb_lo_found
  INC fl_lo
  LDA fl_lo
  CMP #FLEET_COLS
  BCC fb_lo
fb_lo_found:
  LDA #FLEET_COLS-1
  STA fl_hi
fb_hi:
  LDX fl_hi
  JSR column_live
  BNE fb_hi_found
  DEC fl_hi
  BPL fb_hi
fb_hi_found:
  RTS

column_live:
  LDA #FLEET_RANKS
  STA E_R
  TXA
cl_loop:
  TAY
  LDA fl_on, Y
  BNE cl_yes
  TYA
  CLC
  ADC #FLEET_COLS
  DEC E_R
  BNE cl_loop
  LDA #0
  RTS
cl_yes:
  LDA #1
  RTS

// ---------------------------------------------------------------- dive

// Peel dive_pick into a free diver slot. Sets dive_event=1 on success.
.global dive_launch
dive_launch:
  LDA #0
  STA dive_event
  LDX #DIVERS-1
dl_slot:
  LDA dv_on, X
  BEQ dl_free
  DEX
  BPL dl_slot
  RTS
dl_free:
  STX E_SLOT
  LDY dive_pick
  LDA fl_on, Y
  BNE dl_go
  RTS
dl_go:
  LDA #0
  STA fl_on, Y
  // Wipe formation cell (6-col window starts one column left of the pitch).
  LDA idx_col5, Y
  CLC
  ADC fl_col
  SEC
  SBC #1
  STA fx_col
  LDA idx_rank, Y
  ASL A
  CLC
  ADC fl_row
  STA fx_row
  LDA #48
  STA fx_len
  JSR fx_wipe
  LDX E_SLOT
  LDA dive_pick
  STA dv_idx, X
  TAY
  LDA #1
  STA dv_on, X
  // dv_x = (idx_col5[Y] + fl_col) * 2 (Mode-2 pixel, 8-bit).
  LDA idx_col5, Y
  CLC
  ADC fl_col
  ASL A
  STA dv_x, X
  // Character row -> pixel y.
  LDA idx_rank, Y
  ASL A
  CLC
  ADC fl_row
  ASL A
  ASL A
  ASL A
  STA dv_y, X
  // Aim toward the cannon: left or right.
  LDA dv_x, X
  CMP cannon_x
  BCC dl_right
  LDA #256-DIVE_VX
  STA dv_vx, X
  JMP dl_vy
dl_right:
  LDA #DIVE_VX
  STA dv_vx, X
dl_vy:
  LDA #DIVE_VY
  STA dv_vy, X
  LDA #0
  STA dv_phase, X
  LDA #1
  STA dive_event
  JSR diver_paint
  RTS

// Move every diver one step: wipe, advance, draw; set dive_hit / dive_kill.
.global divers_step
divers_step:
  LDA #0
  STA dive_hit
  LDX #DIVERS-1
ds_loop:
  STX E_SLOT
  LDA dv_on, X
  BEQ ds_next
  JSR diver_move
ds_next:
  LDX E_SLOT
  DEX
  BPL ds_loop
  RTS

diver_move:
  // Each step blits a 6-col x 12-scanline padded strip (1 blank col each side,
  // 2 blank scanlines top & bottom) wiping trailing edges in-pass.
  LDX E_SLOT
  // Horizontal (signed add, bounce off the side walls).
  CLC
  LDA dv_x, X
  ADC dv_vx, X
  STA dv_x, X
  CMP #CANNON_MIN
  BCS dm_hi
  LDA #CANNON_MIN
  STA dv_x, X
  LDA #DIVE_VX
  STA dv_vx, X
  JMP dm_fall
dm_hi:
  CMP #CANNON_MAX+1
  BCC dm_fall
  LDA #CANNON_MAX
  STA dv_x, X
  LDA #256-DIVE_VX
  STA dv_vx, X
dm_fall:
  LDA dv_phase, X
  BNE dm_return
  CLC
  LDA dv_y, X
  ADC dv_vy, X
  STA dv_y, X
  CMP #PLAYER_Y
  BCC dm_draw
  // Reached the player band: hit test, then turn for home.
  JSR diver_vs_cannon
  LDX E_SLOT
  LDA #1
  STA dv_phase, X
  LDA #256-DIVE_VY
  STA dv_vy, X
  JMP dm_draw
dm_return:
  SEC
  LDA dv_y, X
  SBC #DIVE_VY
  STA dv_y, X
  BCS dm_home
  JMP diver_rejoin
dm_home:
  LDY dv_idx, X
  LDA idx_rank, Y
  ASL A
  CLC
  ADC fl_row
  ASL A
  ASL A
  ASL A
  STA E_T0
  LDA dv_y, X
  CMP E_T0
  BCS dm_draw
  JMP diver_rejoin
dm_draw:
  JMP diver_paint

diver_wipe:
  LDX E_SLOT
  LDA dv_x, X
  STA E_X
  LDA dv_y, X
  STA fx_y
  JSR window_col
  LDA #8
  STA fx_len
  LDA #6
  STA E_LEFT
dw_col:
  JSR fx_blank
  INC fx_col
  DEC E_LEFT
  BNE dw_col
  RTS

diver_paint:
  LDX E_SLOT
  LDY dv_idx, X
  LDA idx_rank, Y
  ASL A
  ORA fl_frame
  ASL A
  TAY
  LDA alien_spr, Y
  STA E_T0
  LDA alien_spr+1, Y
  STA E_T1
  LDX E_SLOT
  LDA dv_x, X
  STA E_X
  AND #1
  BEQ dp_even
  CLC
  LDA E_T0
  ADC #48
  STA E_T0
  BCC dp_even
  INC E_T1
dp_even:
  JSR window_col
  LDX E_SLOT
  LDA dv_y, X
  SEC
  SBC #2
  STA fx_y
  LDA #12
  STA fx_len
  LDA #%lo(dv_col_buf)
  STA fx_src
  LDA #%hi(dv_col_buf)
  STA fx_src+1
  LDY #0
  LDA #6
  STA E_LEFT
dp_col:
  LDX #0
dp_row:
  LDA (E_T0), Y
  STA dv_col_buf+2, X
  INY
  INX
  CPX #8
  BCC dp_row
  STY E_C4
  JSR fx_strip
  LDY E_C4
  INC fx_col
  DEC E_LEFT
  BNE dp_col
  RTS

diver_vs_cannon:
  LDX E_SLOT
  LDA dv_y, X
  CMP #PLAYER_Y+8
  BCS dvc_out
  CLC
  ADC #8
  CMP #PLAYER_Y
  BCC dvc_out
  SEC
  LDA dv_x, X
  SBC cannon_x
  BCS dvc_pos
  EOR #0xff
  CLC
  ADC #1
dvc_pos:
  CMP #8
  BCS dvc_out
  LDA #1
  STA dive_hit
dvc_out:
  RTS

diver_rejoin:
  JSR diver_wipe
  LDX E_SLOT
  LDY dv_idx, X
  LDA #1
  STA fl_on, Y
  LDA #0
  STA dv_on, X
  STY E_IDX
  JSR alien_at_now
  RTS

alien_at_now:
  LDX E_IDX
  LDA idx_col5, X
  CLC
  ADC fl_col
  SEC
  SBC #1
  STA fx_col
  LDA idx_rank, X
  ASL A
  CLC
  ADC fl_row
  STA fx_row
  LDA idx_rank, X
  ASL A
  ORA fl_frame
  ASL A
  TAY
  LDA alien_spr, Y
  STA fx_src
  LDA alien_spr+1, Y
  STA fx_src+1
  LDA #48
  STA fx_len
  JMP fx_cell

// Wipe every diver and put them back in formation (player death / wave).
.global divers_clear
divers_clear:
  LDX #DIVERS-1
dc_loop:
  STX E_SLOT
  LDA dv_on, X
  BEQ dc_next
  JSR diver_wipe
  LDX E_SLOT
  LDY dv_idx, X
  LDA #1
  STA fl_on, Y
  LDA #0
  STA dv_on, X
  STY E_IDX
  JSR alien_at_now
dc_next:
  LDX E_SLOT
  DEX
  BPL dc_loop
  RTS

.global diver_kill
diver_kill:
  LDX dive_kill_slot
  LDA dv_on, X
  BEQ dk_out
  STX E_SLOT
  JSR diver_wipe
  LDX E_SLOT
  LDY dv_idx, X
  LDA idx_rank, Y
  STA dive_kill_rank
  LDA #0
  STA dv_on, X
  STA fl_on, Y
dk_out:
  RTS

// ---------------------------------------------------------------- laser

// Fire the bolt in slot E_SHOT from the cannon's centre.
shot_fire:
  CLC
  LDA cannon_x
  ADC #4
  STA E_T0
  AND #1
  TAX
  LDA shot_masks, X
  STA E_T1
  LDX E_SHOT
  LDA E_T1
  STA shot_mask, X
  LDA E_T0
  LSR A
  STA shot_col, X
  LDA #SHOT_START
  STA shot_y, X
  LDA #1
  STA shot_on, X
  JSR shot_blit
  LDX #%lo(snd_shot)
  LDY #%hi(snd_shot)
  JMP sound_play

// Build shot_buf (SHOT_LEN lit rows then a dark wipe tail) and strip it at
// the current slot's position.
shot_blit:
  LDX E_SHOT
  LDA shot_mask, X
  STA E_T0
  LDY #0
sb_lit:
  LDA E_T0
  STA shot_buf, Y
  INY
  CPY #SHOT_LEN
  BCC sb_lit
sb_dark:
  LDA #0
  STA shot_buf, Y
  INY
  CPY #SHOT_SPAN
  BCC sb_dark
  LDA #%lo(shot_buf)
  STA fx_src
  LDA #%hi(shot_buf)
  STA fx_src+1
  LDX E_SHOT
  LDA shot_col, X
  STA fx_col
  LDA shot_y, X
  STA fx_y
  LDA #SHOT_SPAN
  STA fx_len
  JMP fx_strip

// C++ sets shot_i to the slot to advance (one of SHOTS bolts) and calls this.
.global shot_step
shot_step:
  LDA #0
  STA shot_event
  LDX shot_i
  STX E_SHOT
  STX cur_slot
  LDA shot_on, X
  BNE ss_go
  RTS
ss_go:
  LDA shot_col, X
  STA cur_col
  LDA shot_y, X
  STA cur_y
  LDA shot_y, X
  SEC
  SBC #SHOT_STEP
  STA E_NY
  CMP #SHOT_TOP
  BCS ss_up
  JMP shot_spent
ss_up:
  LDX E_SHOT
  LDA E_NY
  STA shot_y, X
  STA cur_y
  JSR shot_blit
  JSR hit_divers
  LDA shot_event
  BNE ss_out
  JSR hit_fleet
ss_out:
  RTS

shot_spent:
  LDA #3
  STA shot_event
shot_wipe:
  LDX cur_slot
  LDA #0
  STA shot_on, X
  LDA cur_col
  STA fx_col
  LDA cur_y
  STA fx_y
  LDA #SHOT_LEN
  STA fx_len
  JMP fx_blank

hit_divers:
  LDX #DIVERS-1
hd_loop:
  STX E_SLOT
  LDA dv_on, X
  BEQ hd_next
  LDA dv_x, X
  LSR A
  STA E_C
  LDA cur_col
  SEC
  SBC E_C
  CLC
  ADC #1
  CMP #4
  BCS hd_next
  SEC
  LDA cur_y
  SBC dv_y, X
  CLC
  ADC #5
  CMP #13
  BCS hd_next
  LDA #2
  STA shot_event
  LDX E_SLOT
  STX dive_kill_slot
  JSR shot_wipe
  JMP diver_kill
hd_next:
  LDX E_SLOT
  DEX
  BPL hd_loop
  RTS

hit_fleet:
  LDA cur_col
  SEC
  SBC fl_col
  BCC hf_miss
  STA E_C4
  LDX #0
hf_div:
  CMP #5
  BCC hf_col
  SBC #5
  INX
  JMP hf_div
hf_col:
  CPX #FLEET_COLS
  BCS hf_miss
  STX E_C
  LDA cur_y
  LSR A
  LSR A
  LSR A
  SEC
  SBC fl_row
  BCC hf_miss
  LSR A
  CMP #FLEET_RANKS
  BCS hf_miss
  STA E_RANK
  ASL A
  ASL A
  ASL A
  CLC
  ADC E_C
  TAY
  LDA fl_on, Y
  BEQ hf_miss
  LDA #0
  STA fl_on, Y
  DEC alive
  LDA #1
  STA shot_event
  LDA E_C
  STA E_T0
  ASL A
  ASL A
  CLC
  ADC E_T0
  CLC
  ADC fl_col
  SEC
  SBC #1
  STA shot_hit_col
  STA fx_col
  LDA E_RANK
  ASL A
  CLC
  ADC fl_row
  STA shot_hit_row
  STA fx_row
  LDA E_RANK
  STA shot_hit_rank
  LDA #48
  STA fx_len
  JSR fx_wipe
  JMP shot_wipe
hf_miss:
  RTS

// ---------------------------------------------------------------- bombs

.global bomb_drop
bomb_drop:
  LDX #BOMBS-1
bd_slot:
  LDA bomb_on, X
  BEQ bd_free
  DEX
  BPL bd_slot
  RTS
bd_free:
  STX E_IDX
  // Prefer a diving alien's x; else a random formation column.
  LDY #DIVERS-1
bd_diver:
  LDA dv_on, Y
  BNE bd_from_diver
  DEY
  BPL bd_diver
  LDA drop_col
  CMP #FLEET_COLS
  BCC bd_have
  LDA fl_lo
bd_have:
  STA E_C
  LDA E_C
  STA E_T0
  ASL A
  ASL A
  CLC
  ADC E_T0
  CLC
  ADC fl_col
  STA bomb_col, X
  LDA fl_row
  CLC
  ADC #FLEET_RANKS
  ADC #FLEET_RANKS
  ASL A
  ASL A
  ASL A
  STA bomb_y, X
  JMP bd_arm
bd_from_diver:
  LDA dv_x, Y
  LSR A
  STA bomb_col, X
  LDA dv_y, Y
  CLC
  ADC #8
  STA bomb_y, X
bd_arm:
  LDA #0
  STA bomb_anim, X
  LDA #1
  STA bomb_on, X
  RTS

.global bombs_step
bombs_step:
  LDA #0
  STA bomb_hit
  LDX #BOMBS-1
bs_loop:
  STX E_IDX
  LDA bomb_on, X
  BEQ bs_next
  JSR bomb_move
bs_next:
  LDX E_IDX
  DEX
  BPL bs_loop
  RTS

bomb_move:
  LDA bomb_y, X
  CLC
  ADC #BOMB_STEP
  STA E_NY
  ADC #BOMB_LEN
  CMP #GROUND_Y+1
  BCC bm_cannon
  JMP bomb_spent
bm_cannon:
  LDA E_NY
  CMP #PLAYER_Y+8
  BCS bm_draw
  CLC
  ADC #BOMB_LEN
  CMP #PLAYER_Y
  BCC bm_draw
  LDA bomb_col, X
  ASL A
  STA E_T0
  SEC
  SBC cannon_x
  BCS bc_pos
  EOR #0xff
  CLC
  ADC #1
bc_pos:
  CMP #8
  BCS bm_draw
  LDA #1
  STA bomb_hit
  JMP bomb_spent
bm_draw:
  LDA bomb_col, X
  STA fx_col
  LDA bomb_y, X
  STA fx_y
  LDA #BOMB_LEN
  STA fx_len
  JSR fx_blank
  LDX E_IDX
  LDA E_NY
  STA bomb_y, X
  INC bomb_anim, X
  LDA bomb_anim, X
  AND #2
  LSR A
  TAY
  TYA
  ASL A
  TAY
  LDA bomb_pat, Y
  STA fx_src
  LDA bomb_pat+1, Y
  STA fx_src+1
  LDA bomb_col, X
  STA fx_col
  LDA bomb_y, X
  STA fx_y
  LDA #BOMB_LEN
  STA fx_len
  JMP fx_strip

bomb_spent:
  LDA #0
  STA bomb_on, X
  LDA bomb_y, X
  STA fx_y
  LDA bomb_col, X
  STA fx_col
  LDA #BOMB_LEN
  STA fx_len
  JMP fx_blank

// ---------------------------------------------------------------- sprites

window_draw:
  LDA E_X
  AND #1
  BEQ wd_even
  CLC
  LDA fx_src
  ADC #48
  STA fx_src
  BCC wd_even
  INC fx_src+1
wd_even:
  JSR window_col
  LDA #48
  STA fx_len
  JMP fx_cell

window_wipe:
  JSR window_col
  LDA #48
  STA fx_len
  JMP fx_wipe

window_col:
  LDA E_X
  LSR A
  SEC
  SBC #1
  STA fx_col
  RTS

cannon_at:
  LDA cannon_x
  STA E_X
  LDA #PLAYER_ROW
  STA fx_row
  RTS

.global player_step
player_step:
  LDA key_bits
  AND #KEY_FIRE
  ORA fire_want
  STA fire_want
  LDA key_bits
  LSR A
  BCS ps_left
  LSR A
  BCS ps_right
  BCC ps_fire
ps_left:
  LDA cannon_x
  CMP #CANNON_MIN+1
  BCC ps_fire
  SEC
  SBC #CANNON_STEP
  STA cannon_x
  JMP ps_moved
ps_right:
  LDA cannon_x
  CMP #CANNON_MAX
  BCS ps_fire
  CLC
  ADC #CANNON_STEP
  STA cannon_x
ps_moved:
  JSR cannon_draw
ps_fire:
  LDA fire_want
  BEQ ps_out
  LDX #SHOTS-1
pf_slot:
  LDA shot_on, X
  BEQ pf_free
  DEX
  BPL pf_slot
  BMI ps_out
pf_free:
  LDA #0
  STA fire_want
  STX E_SHOT
  JMP shot_fire
ps_out:
  RTS

.global cannon_draw
cannon_draw:
  LDA #%lo(k_ga_ship)
  STA fx_src
  LDA #%hi(k_ga_ship)
  STA fx_src+1
.global cannon_image
cannon_image:
  JSR cannon_at
  JMP window_draw

.global cannon_erase
cannon_erase:
  JSR cannon_at
  JMP window_wipe

// ---------------------------------------------------------------- sound

sound_play:
  LDA #7
  JMP OSWORD

.global sound_boom
sound_boom:
  LDX #%lo(snd_boom)
  LDY #%hi(snd_boom)
  JMP sound_play

.global sound_death
sound_death:
  LDX #%lo(snd_death)
  LDY #%hi(snd_death)
  JMP sound_play

.global sound_dive
sound_dive:
  LDX #%lo(snd_dive)
  LDY #%hi(snd_dive)
  JMP sound_play
