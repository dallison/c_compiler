// Galaxian play for galaxians.cc: each frame's keys, cannon, convoy sway,
// divers, laser, bombs, and their collisions. Lives in .text.blitter so it
// stays below &3000 next to screen1.s. Units and layout are in galaxians.h.

#include "galaxians.h"

.set OSWORD 0xfff1
.set OSBYTE 0xfff4
.set SYSVIA_LATCH 0xfe40
.set SYSVIA_DDRA 0xfe43
.set SYSVIA_PORTA 0xfe4f

.set E_NY 0x78
.set E_IDX 0x79
.set E_OC 0x7a
.set E_OR 0x7b
.set E_C 0x7c
.set E_R 0x7d
.set E_T0 0x7e
.set E_T1 0x7f
.set E_LEFT 0x80
.set E_C4 0x81
.set E_RANK 0x82
.set E_X 0x83
.set E_SLOT 0x85

.section ".text.blitter", "ax", @progbits

shot_pat:
.space SHOT_SPAN

// ---------------------------------------------------------------- misc

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
  LDA lives
  LDX #HUD_LIVES_COL
  JSR hud_unit
  LDA wave
  LDX #HUD_WAVE_COL
hud_unit:
  STX E_C
  CMP #10
  BCC hud_glyph
  LDA #9
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
  LDA #HUD_ROW
  STA fx_row
  LDA #16
  STA fx_len
  JMP fx_cell

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

// Redraw up to FLEET_QUOTA live aliens at the current origin. Between
// sweeps fl_mover is 255; fleet_sway starts a new sweep after a step.
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

// Nudge the convoy one byte column; reverse at the edges. Edge tests use
// the 5-column pitch so the 6-column windows stay on-screen.
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

// X = alien. Draw at the current origin first. The 6-column window's blank
// pads clear the trailing edge on a 1-column sway, so a full wipe (which
// tore neighbours on a tight pitch) is not used for horizontal steps.
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
  BEQ dl_out
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
  // Pixel x of the formation slot (byte col * 4).
  LDA idx_col5, Y
  CLC
  ADC fl_col
  ASL A
  ASL A
  STA dv_x, X
  LDA #0
  STA dv_xh, X
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
  LDA cannon_x
  CMP dv_x, X
  BCS dl_right
  LDA #256-2
  STA dv_vx, X
  JMP dl_vy
dl_right:
  LDA #2
  STA dv_vx, X
dl_vy:
  LDA #2
  STA dv_vy, X
  LDA #0
  STA dv_phase, X
  LDA #1
  STA dive_event
dl_out:
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
  // Remember the old cell; paint the new pose first. Wipe the old cell
  // only when the character row changes (no vertical overlap). Horizontal
  // steps rely on the window's blank pads, like the convoy sway.
  JSR diver_cell
  LDA fx_col
  STA E_OC
  LDA fx_row
  STA E_OR
  LDX E_SLOT
  // Horizontal.
  LDA dv_vx, X
  BMI dm_left
  CLC
  LDA dv_x, X
  ADC dv_vx, X
  STA dv_x, X
  BCC dm_y
  INC dv_xh, X
  JMP dm_y
dm_left:
  SEC
  LDA dv_x, X
  SBC #2
  STA dv_x, X
  BCS dm_y
  LDA dv_xh, X
  BEQ dm_clamp_l
  DEC dv_xh, X
  JMP dm_y
dm_clamp_l:
  LDA #0
  STA dv_x, X
dm_y:
  // Bounce off the side walls.
  LDA dv_xh, X
  BNE dm_right_ok
  LDA dv_x, X
  CMP #CANNON_MIN
  BCS dm_right_ok
  LDA #2
  STA dv_vx, X
dm_right_ok:
  LDA dv_xh, X
  BEQ dm_fall
  LDA dv_x, X
  CMP #(CANNON_MAX & 0xff)
  BCC dm_fall
  LDA #256-2
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
  LDA #256-2
  STA dv_vy, X
  JMP dm_draw
dm_return:
  SEC
  LDA dv_y, X
  SBC #2
  STA dv_y, X
  BCS dm_home
  // Off the top: put back in formation.
  JMP diver_rejoin
dm_home:
  // Snap home when near formation row for this alien.
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
  JSR diver_paint
  JSR diver_cell
  LDA fx_row
  CMP E_OR
  BEQ dm_out
  LDA E_OC
  STA fx_col
  LDA E_OR
  STA fx_row
  LDA #48
  STA fx_len
  JMP fx_wipe
dm_out:
  RTS

diver_cell:
  LDX E_SLOT
  LDA dv_xh, X
  LSR A
  LDA dv_x, X
  ROR A
  LSR A
  SEC
  SBC #1
  STA fx_col
  LDA dv_y, X
  LSR A
  LSR A
  LSR A
  STA fx_row
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
  STA fx_src
  LDA alien_spr+1, Y
  STA fx_src+1
  LDA dv_x, X
  STA E_X
  LDA dv_xh, X
  STA E_X+1
  LDA dv_y, X
  LSR A
  LSR A
  LSR A
  STA fx_row
  JMP window_draw

diver_vs_cannon:
  LDX E_SLOT
  LDA dv_y, X
  CMP #PLAYER_Y+8
  BCS dvc_out
  ADC #8
  CMP #PLAYER_Y+1
  BCC dvc_out
  LDA #0
  STA E_T1
  LDA dv_x, X
  STA E_T0
  LDA dv_xh, X
  STA E_T1
  CLC
  LDA E_T0
  ADC #8
  STA E_T0
  LDA E_T1
  ADC #0
  STA E_T1
  SEC
  LDA E_T0
  SBC cannon_x
  STA E_T0
  LDA E_T1
  SBC cannon_x+1
  BNE dvc_out
  LDA E_T0
  CMP #24
  BCS dvc_out
  LDA #1
  STA dive_hit
dvc_out:
  RTS

diver_rejoin:
  LDX E_SLOT
  LDY dv_idx, X
  LDA #1
  STA fl_on, Y
  LDA #0
  STA dv_on, X
  // Redraw in formation.
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

// Wipe and clear diver E_SLOT; caller awards score via dive_kill_rank.
// Wipe every diver and put them back in formation (player death / wave).
.global divers_clear
divers_clear:
  LDX #DIVERS-1
dc_loop:
  STX E_SLOT
  LDA dv_on, X
  BEQ dc_next
  JSR diver_cell
  LDA #48
  STA fx_len
  JSR fx_wipe
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
  JSR diver_cell
  LDA #48
  STA fx_len
  JSR fx_wipe
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

shot_fire:
  LDA cannon_x
  CLC
  ADC #8
  STA shot_x
  LDA cannon_x+1
  ADC #0
  // byte column of the shot
  LSR A
  LDA shot_x
  ROR A
  LSR A
  STA shot_col
  LDA #SHOT_START
  STA shot_y
  LDA #1
  STA shot_on
  // Build shot_pat: SHOT_LEN lit rows then dark wipe.
  LDA shot_x
  AND #3
  TAX
  LDA shot_masks, X
  STA E_T0
  LDX #0
sf_lit:
  LDA E_T0
  STA shot_pat, X
  INX
  CPX #SHOT_LEN
  BCC sf_lit
sf_dark:
  LDA #0
  STA shot_pat, X
  INX
  CPX #SHOT_SPAN
  BCC sf_dark
  LDA #%lo(shot_pat)
  STA fx_src
  LDA #%hi(shot_pat)
  STA fx_src+1
  LDA shot_col
  STA fx_col
  LDA shot_y
  STA fx_y
  LDA #SHOT_SPAN
  STA fx_len
  JSR fx_strip
  LDX #%lo(snd_shot)
  LDY #%hi(snd_shot)
  JMP sound_play

shot_masks:
.byte 0x88, 0x44, 0x22, 0x11

.global shot_step
shot_step:
  LDA #0
  STA shot_event
  LDA shot_on
  BNE ss_go
  RTS
ss_go:
  LDA shot_y
  SEC
  SBC #SHOT_STEP
  STA E_NY
  CMP #SHOT_TOP
  BCS ss_up
  JMP shot_spent
ss_up:
  STA shot_y
  LDA #%lo(shot_pat)
  STA fx_src
  LDA #%hi(shot_pat)
  STA fx_src+1
  LDA shot_col
  STA fx_col
  LDA shot_y
  STA fx_y
  LDA #SHOT_SPAN
  STA fx_len
  JSR fx_strip
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
  LDA #0
  STA shot_on
  LDA shot_col
  STA fx_col
  LDA shot_y
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
  // Same byte column band?
  LDA dv_xh, X
  LSR A
  LDA dv_x, X
  ROR A
  LSR A
  STA E_C
  LDA shot_col
  SEC
  SBC E_C
  CLC
  ADC #1
  CMP #4
  BCS hd_next
  LDA dv_y, X
  CMP shot_y
  BCC hd_next
  SEC
  SBC #8
  CMP shot_y
  BCS hd_next
  // Hit.
  LDA #2
  STA shot_event
  STX dive_kill_slot
  JSR shot_wipe
  JMP diver_kill
hd_next:
  LDX E_SLOT
  DEX
  BPL hd_loop
  RTS

hit_fleet:
  // shot_col vs formation cell on the 5-column pitch.
  LDA shot_col
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
  // Which rank from shot_y.
  LDA shot_y
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
  // Window origin is pitch - 1 (leading blank column).
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
  // bomb_col = fl_col + 5 * column
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
  LDA dv_xh, Y
  LSR A
  LDA dv_x, Y
  ROR A
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
  ADC #BOMB_LEN
  CMP #PLAYER_Y+1
  BCC bm_draw
  LDA #0
  STA E_T1
  LDA bomb_col, X
  ASL A
  ROL E_T1
  ASL A
  ROL E_T1
  CLC
  ADC #3
  STA E_T0
  LDA E_T1
  ADC #0
  STA E_T1
  SEC
  LDA E_T0
  SBC cannon_x
  STA E_T0
  LDA E_T1
  SBC cannon_x+1
  BNE bm_draw
  LDA E_T0
  CMP #19
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
  AND #2
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
  LDA E_X+1
  LSR A
  LDA E_X
  ROR A
  LSR A
  SEC
  SBC #1
  STA fx_col
  RTS

cannon_at:
  LDA cannon_x
  STA E_X
  LDA cannon_x+1
  STA E_X+1
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
  LDA cannon_x+1
  BNE ps_go_left
  LDA cannon_x
  CMP #CANNON_MIN+1
  BCC ps_fire
ps_go_left:
  SEC
  LDA cannon_x
  SBC #CANNON_STEP
  STA cannon_x
  LDA cannon_x+1
  SBC #0
  STA cannon_x+1
  JMP ps_moved
ps_right:
  LDA cannon_x+1
  BEQ ps_go_right
  LDA cannon_x
  CMP #(CANNON_MAX & 0xff)
  BCS ps_fire
ps_go_right:
  CLC
  LDA cannon_x
  ADC #CANNON_STEP
  STA cannon_x
  LDA cannon_x+1
  ADC #0
  STA cannon_x+1
ps_moved:
  JSR cannon_draw
ps_fire:
  LDA fire_want
  BEQ ps_out
  LDA shot_on
  BNE ps_out
  STA fire_want
  JMP shot_fire
ps_out:
  RTS

.global cannon_draw
cannon_draw:
  LDA #%lo(k_gx_ship)
  STA fx_src
  LDA #%hi(k_gx_ship)
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
